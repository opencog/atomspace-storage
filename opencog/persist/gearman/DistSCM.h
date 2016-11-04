/*
 * DistSCM.h
 *
 * Copyright (C) 2015 OpenCog Foundation
 *
 * Author: Mandeep Singh Bhatia , September 2015
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <string>

#include <libgearman/gearman.h>

#include <opencog/guile/SchemeModule.h>

namespace opencog {

class DistSCM : public ModuleWrap
{
private:

	void init(void);

	// XXX FIXME -- a single global vairable? This cannot be right!
	static bool master_mode;
	void set_master_mode(void);
	const std::string& slave_mode(const std::string& ip_string,
	                              const std::string& workerID);
	UUID dist_scm(const std::string& scm_string,
	              const std::string& clientID,
	              bool truth);

	// XXX FIXME -- a single client and worker? This cannot be right!
	gearman_client_st client;
	gearman_worker_st *worker;
	static gearman_return_t worker_function(gearman_job_st *job, void *context);

public:
	DistSCM(void);
	~DistSCM();
};

}

extern "C" {
void opencog_dist_init(void);
};
