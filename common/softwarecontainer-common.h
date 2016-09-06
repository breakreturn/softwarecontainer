
/*
 * Copyright (C) 2016 Pelagicore AB
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * For further information see LICENSE
 */


#pragma once

#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <map>
#include <memory>
#include <vector>

#include "softwarecontainer-config.h"
#include "softwarecontainer-log.h"

namespace softwarecontainer {

typedef uint32_t ContainerID;

static constexpr ContainerID INVALID_CONTAINER_ID = -1;
static constexpr pid_t INVALID_PID = -1;
static constexpr int INVALID_FD = -1;

typedef std::map<std::string, std::string> GatewayConfiguration;

enum class ContainerState
{
    CREATED,
    PRELOADED,
    READY,
    //    RUNNING,
    TERMINATED
};

enum class ReturnCode
{
    FAILURE,
    SUCCESS
};

inline ReturnCode bool2ReturnCode(bool b)
{
    return b ? ReturnCode::SUCCESS : ReturnCode::FAILURE;
}

inline bool isError(ReturnCode code)
{
    return (code != ReturnCode::SUCCESS);
}

inline bool isSuccess(ReturnCode code)
{
    return !isError(code);
}

typedef std::map<std::string, std::string> EnvironmentVariables;

static constexpr const char *AGENT_OBJECT_PATH = "/com/pelagicore/SoftwareContainerAgent";
static constexpr const char *AGENT_BUS_NAME = "com.pelagicore.SoftwareContainerAgent";

static constexpr uid_t ROOT_UID = 0;

/**
 * That class contains references to sigc++ connections and automatically disconnects them on destruction
 */
class SignalConnectionsHandler
{

public:
    /**
     * Add a new connection
     */
    void addConnection(sigc::connection &connection) {
        m_connections.push_back(connection);
    }

    ~SignalConnectionsHandler()
    {
        for (auto &connection : m_connections) {
            connection.disconnect();
        }
    }

private:
    std::vector<sigc::connection> m_connections;

};

inline void addProcessListener(SignalConnectionsHandler &connections,
                               pid_t pid, std::function<void(pid_t, int)> function,
                               Glib::RefPtr<Glib::MainContext> context)
{
    Glib::SignalChildWatch watch = context->signal_child_watch();
    auto connection = watch.connect(function,pid);
    connections.addConnection(connection);
}

inline int waitForProcessTermination(pid_t pid)
{
    int status = 0;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

/*
 * Check if path is a directory
 */
bool isDirectory(const std::string &path);
bool isFile(const std::string &path);
bool isPipe(const std::string &path);
bool isSocket(const std::string &path);

bool existsInFileSystem(const std::string &path);

std::string parentPath(const std::string &path);
ReturnCode touch(const std::string &path);
ReturnCode writeToFile(const std::string &path, const std::string &content);
ReturnCode readFromFile(const std::string &path, std::string &content);

template<typename Type>
class ObservableProperty
{
public:
    typedef std::function<void (const Type &)> Listener;

    ObservableProperty(Type &value) :
        m_value(value)
    {
    }

    void addListener(Listener listener)
    {
        m_listeners.push_back(listener);
    }

    operator const Type &() {
        return m_value;
    }

protected:
    std::vector<Listener> m_listeners;

private:
    const Type &m_value;

};

template<typename Type>
class ObservableWritableProperty :
    public ObservableProperty<Type>
{
public:
    ObservableWritableProperty(Type value) :
        ObservableProperty<Type>(m_value), m_value(value)
    {
    }

    ObservableWritableProperty() :
        ObservableProperty<Type>(m_value)
    {
    }

    void setValueNotify(Type value)
    {
        m_value = value;
        for (auto &listener : ObservableProperty<Type>::m_listeners) {
            listener(getValue());
        }
    }

    const Type &getValue() const
    {
        return m_value;
    }

    ObservableWritableProperty &operator=(const Type &type)
    {
        m_value = type;
        return *this;
    }

private:
    Type m_value;

};

class CleanUpHandler
{
protected:
    LOG_DECLARE_CLASS_CONTEXT("CLEA", "Cleanup handler");
public:
    virtual ~CleanUpHandler()
    {
    }
    virtual ReturnCode clean() = 0;
};

class DirectoryCleanUpHandler :
    public CleanUpHandler
{
public:
    DirectoryCleanUpHandler(const std::string &path)
    {
        m_path = path;
    }

    ReturnCode clean() override
    {
        if (!existsInFileSystem(m_path)) {
            log_warning() << "Folder " << m_path << " does not exist";
            return ReturnCode::SUCCESS;
        }

        if (rmdir(m_path.c_str()) == 0) {
            log_debug() << "rmdir'd " << m_path;
            return ReturnCode::SUCCESS;
        } else {
            log_error() << "Can't rmdir " << m_path << " . Error :" << strerror(errno);
            return ReturnCode::FAILURE;
        }
    }

    std::string m_path;
};

class FileCleanUpHandler :
    public CleanUpHandler
{
public:
    FileCleanUpHandler(const std::string &path)
    {
        m_path = path;
    }

    ReturnCode clean() override
    {
        if (unlink(m_path.c_str()) == 0) {
            log_debug() << "Unlinked " << m_path;
            return ReturnCode::SUCCESS;
        } else {
            log_error() << "Can't delete " << m_path << " . Error :" << strerror(errno);
            return ReturnCode::FAILURE;
        }
    }

    std::string m_path;
};

class MountCleanUpHandler :
    public CleanUpHandler
{
public:
    MountCleanUpHandler(const std::string &path)
    {
        m_path = path;
    }

    ReturnCode clean() override
    {
        // Lazy unmount. Should be the equivalent of the "umount -l" command.
        if (umount2(m_path.c_str(), MNT_DETACH) == 0) {
            log_debug() << "Unmounted " << m_path;
            return ReturnCode::SUCCESS;
        } else {
            log_warn() << "Can't unmount " << m_path << " . Error :" << strerror(errno) << ". Trying to force umount";
            if (umount2(m_path.c_str(), MNT_FORCE) != 0) {
                log_warn() << "Can't force unmount " << m_path << " . Error :" << strerror(errno);
                return ReturnCode::FAILURE;
            }
            log_debug() << "Managed to force unmount " << m_path;
            return ReturnCode::SUCCESS;
        }
    }
    std::string m_path;
};

class FileToolkitWithUndo
{
    LOG_DECLARE_CLASS_CONTEXT("CLEA", "File toolkit");

public:
    ~FileToolkitWithUndo()
    {
        bool success = true;
        // Clean up all created directories, files, and mount points
        for (auto it = m_cleanupHandlers.rbegin(); it != m_cleanupHandlers.rend(); ++it) {
            // Cleaning functions will do their own error/warning output
            if(isError((*it)->clean())) {
                success = false;
            }
            delete *it;
        }

        if(!success) {
            log_error() << "One or more cleanup handlers returned error status, please check the log";
        }
    }

    ReturnCode createParentDirectory(const std::string &path)
    {
        log_debug() << "Creating parent directories for " << path;
        std::string parent = parentPath(path);
        if (!isDirectory(parent) && parent != "") {
            if(isError(createDirectory(parent))) {
                log_error() << "Could not create directory " << parent;
                return ReturnCode::FAILURE;
            }
        }
        return ReturnCode::SUCCESS;
    }

    /**
     * Create a directory, and if successful append it to a list of dirs
     * to be deleted in the dtor. Since nestled dirs will need to be
     * deleted in reverse order to creation insert to the beginning of
     * the list.
     */
    ReturnCode createDirectory(const std::string &path)
    {
        if (isDirectory(path)) {
            return ReturnCode::SUCCESS;
        }

        if(isError(createParentDirectory(path))) {
            log_error() << "Couldn't create parent directory for " << path;
            return ReturnCode::FAILURE;
        }

        if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
            log_error() << "Could not create directory " << path << " - Reason : " << strerror(errno);
            return ReturnCode::FAILURE;
        }

        m_cleanupHandlers.push_back(new DirectoryCleanUpHandler(path));
        log_debug() << "Created directory " << path;

        return ReturnCode::SUCCESS;
    }

    ReturnCode bindMount(const std::string &src, const std::string &dst, bool readOnly)
    {
        unsigned long flags = MS_BIND;
        const char *fstype = nullptr;
        const void *data = nullptr;
        log_debug() << "Bind-mounting " << src << " in " << dst << ", flags: " << flags;

        int mountRes = mount(src.c_str(), dst.c_str(), fstype, flags, data);
        if (mountRes == 0) {
            log_verbose() << "Bind-mounted folder " << src << " in " << dst;
            m_cleanupHandlers.push_back(new MountCleanUpHandler(dst));
        } else {
            log_error() << "Could not mount into container: src=" << src
                        << " , dst=" << dst << " err=" << strerror(errno);
            return ReturnCode::FAILURE;
        }

        if (readOnly) {
            flags = MS_REMOUNT | MS_RDONLY | MS_BIND;

            log_debug() << "Re-mounting read-only" << src << " in "
                        << dst << ", flags: " << flags;
            mountRes = mount(src.c_str(), dst.c_str(), fstype, flags, data);
            if (mountRes != 0) {
                // Failure
                log_error() << "Could not re-mount " << src << " , read-only on "
                            << dst << " err=" << strerror(errno);
                return ReturnCode::FAILURE;
            }
        }
        return ReturnCode::SUCCESS;
    }

    ReturnCode createSharedMountPoint(const std::string &path)
    {
        // MS_MGC_VAL |
        auto mountRes = mount(path.c_str(), path.c_str(), "", MS_BIND, nullptr);
        assert(mountRes == 0);
        mountRes = mount(path.c_str(), path.c_str(), "", MS_UNBINDABLE, nullptr);
        assert(mountRes == 0);
        mountRes = mount(path.c_str(), path.c_str(), "", MS_SHARED, nullptr);
        assert(mountRes == 0);
        m_cleanupHandlers.push_back(new MountCleanUpHandler(path));
        log_debug() << "Created shared mount point at " << path;

        return ReturnCode::SUCCESS;
    }

    ReturnCode writeToFile(const std::string &path, const std::string &content)
    {
        auto ret = softwarecontainer::writeToFile(path, content);
        if (isError(ret)) {
            return ret;
        }
        m_cleanupHandlers.push_back(new FileCleanUpHandler(path));
        log_debug() << "Successfully wrote to " << path;
        return ReturnCode::SUCCESS;
    }

    ReturnCode createSymLink(const std::string &source, const std::string &destination)
    {
        log_debug() << "creating symlink " << source << " pointing to " << destination;

        createDirectory(parentPath(source));

        if (symlink(destination.c_str(), source.c_str()) == 0) {
            m_cleanupHandlers.push_back(new FileCleanUpHandler(source));
            log_debug() << "Successfully created symlink from " << source << " to " << destination;
        } else {
            log_error() << "Error creating symlink " << destination
                        << " pointing to " << source << ". Error: "
                        << strerror(errno);
            return ReturnCode::FAILURE;
        }
        return ReturnCode::SUCCESS;
    }

protected:
    std::vector<CleanUpHandler *> m_cleanupHandlers;
};

}

using namespace softwarecontainer;
using logging::StringBuilder;