#ifndef PELAGICONTAIN_COMMON_H
#define PELAGICONTAIN_COMMON_H

struct lxc_params {

	/* networking */
	char *ip_addr;
	char *gw_addr;
	char *net_iface_name;

	/* traffic control */
	char *tc_rate;

	/* pulse audio */
	char pulse_socket[1024];
	char deployed_pulse_socket[1024];

	/* D-Bus */
	char session_proxy_socket[1024];
	char system_proxy_socket[1024];
	char deployed_session_proxy_socket[1024];
	char deployed_system_proxy_socket[1024];

	/* LXC general */
	char *container_name;
	char *lxc_system_cfg;
	char *ct_dir;			/* directory specified by user */
	char ct_conf_dir[1024];		/* $ct_dir/config/ */
	char ct_root_dir[1024];		/* $ct_dir/rootfs */
	char lxc_cfg_file[1024];

	/* general */
	char main_cfg_file[1024];

};

#endif /* PELAGICONTAIN_COMMON_H */
