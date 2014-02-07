/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#ifndef PELAGICONTAIN_H
#define PELAGICONTAIN_H

#include <sys/types.h>

#include "config.h"
#include "container.h"
#include "paminterface.h"
#include "controllerinterface.h"

class Pelagicontain {
public:
	Pelagicontain(PAMAbstractInterface *pamInterface);
	~Pelagicontain();
	static int initializeConfig(struct lxc_params *ct_pars, const char *ct_base_dir, Config *config);
	int initialize(struct lxc_params &ct_pars, Config &config);
	pid_t run(int numParameters, char **parameters, struct lxc_params *ct_pars,
		const std::string &cookie);
	void launch(const std::string &appId);
	void update(const std::map<std::string, std::string> &configs);
	void shutdown();

private:
	void setGatewayConfigs(const std::map<std::string, std::string> &configs);
	void activateGateways();
	void shutdownGateways();

	ControllerInterface m_controller;
	Container m_container;
	PAMAbstractInterface *m_pamInterface;
	std::vector<Gateway *> m_gateways;
	std::string m_appId;
	std::string m_cookie;
};

#endif /* PELAGICONTAIN_H */
