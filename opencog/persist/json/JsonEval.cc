/**
 * JsonEval.cc
 *
 * Javascript/JSON evaluator.
 *
 * Copyright (c) 2008, 2014, 2015, 2020, 2021 Linas Vepstas
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

#include <chrono>
#include <thread>

#include <opencog/util/Logger.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/json/JSCommands.h>

#include "JsonEval.h"

using namespace opencog;

JsonEval::JsonEval(const AtomSpacePtr& as)
	: GenericEval()
{
	_atomspace = as;
}

JsonEval::~JsonEval()
{
}

/* ============================================================== */
/**
 * Evaluate a Javascript/JSON command.
 */
void JsonEval::eval_expr(const std::string &expr)
{
	try {
		std::lock_guard<std::mutex> lock(_mtx);
		_answer = JSCommands::interpret_command(_atomspace.get(), expr);
	}
	catch (const StandardException& ex)
	{
		_error_string = ex.what();
		_caught_error = true;
	}
}

std::string JsonEval::poll_result()
{
	std::string ret;
	std::lock_guard<std::mutex> lock(_mtx);
	ret.swap(_answer);
	return ret;
}

void JsonEval::begin_eval()
{
	std::unique_lock<std::mutex> lock(_mtx);
	if (_answer.empty()) return;

	// Unusual race condition: some other thread is using this evaluator,
	// and evaluation there has not completed. This seems to be rare,
	// and I think we can just ignore this, at least for a little while.
	auto start = std::chrono::steady_clock::now();
	bool warned = false;
	while (not _answer.empty())
	{
		// Release lock and yield so poll_result() can clear _answer
		lock.unlock();
		std::this_thread::yield();
		lock.lock();

		auto elapsed = std::chrono::steady_clock::now() - start;
		auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

		// Warn if we've been waiting a long time (> 1 second)
		if (not warned and secs >= 1)
		{
			logger().warn("JsonEval::begin_eval: Buffer not empty after 1 sec, size=%lu",
				 _answer.size());
			warned = true;
		}

		// Give up after 60 seconds - something is very wrong
		if (secs >= 60)
		{
			logger().error("JsonEval::begin_eval: Giving up after 60 sec, size=%lu",
				_answer.size());
			return;
		}
	}
}

/* ============================================================== */

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void JsonEval::interrupt(void)
{
	_caught_error = true;
	_error_string = "Caught interrupt!";
}

JsonEval* JsonEval::get_evaluator(const AtomSpacePtr& as)
{
	static thread_local JsonEval* evaluator = new JsonEval(as);

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
