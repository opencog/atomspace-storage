/**
 * SexprEval.cc
 *
 * S-expression evaluator.
 *
 * Copyright (c) 2008, 2014, 2015, 2020 Linas Vepstas
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

#include <opencog/util/Logger.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/sexpr/Commands.h>

#include "SexprEval.h"

using namespace opencog;

SexprEval::SexprEval(AtomSpace* as)
	: GenericEval()
{
	_atomspace = as;
}

SexprEval::~SexprEval()
{
}

/* ============================================================== */
/**
 * Evaluate an s-expresion.
 */
void SexprEval::eval_expr(const std::string &expr)
{
}

std::string SexprEval::poll_result()
{
	return _answer;
}


void SexprEval::begin_eval()
{
	_answer.clear();
}

/* ============================================================== */

/**
 * interrupt() - convert user's control-C at keyboard into exception.
 */
void SexprEval::interrupt(void)
{
	throw IOException(TRACE_INFO, "Caught interrupt!");
}

SexprEval* SexprEval::get_evaluator(AtomSpace* as)
{
	static thread_local SexprEval* evaluator = new SexprEval(as);

	// The eval_dtor runs when this thread is destroyed.
	class eval_dtor {
		public:
		~eval_dtor() { delete evaluator; }
	};
	static thread_local eval_dtor killer;

	return evaluator;
}

/* ===================== END OF FILE ======================== */
