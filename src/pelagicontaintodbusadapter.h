/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#ifndef PELAGICONTAINTODBUSADAPTER_H
#define PELAGICONTAINTODBUSADAPTER_H

#include <dbus-c++/dbus.h>

#include "dbusadaptor.h"
#include "pelagicontain.h"

class PelagicontainToDBusAdapter : 
	public com::pelagicore::Pelagicontain_adaptor,
	public DBus::IntrospectableAdaptor,
	public DBus::ObjectAdaptor
{
public:
	PelagicontainToDBusAdapter(DBus::Connection &conn, const std::string &objPath,
		Pelagicontain &pc);
	virtual std::string Echo(const std::string& argument);
	virtual void Launch(const std::string& appId);
	virtual void Update(const std::map<std::string, std::string> &configs);
	virtual void Shutdown();

private:
	Pelagicontain *m_pelagicontain;
};

#endif /* PELAGICONTAINTODBUSADAPTER_H */