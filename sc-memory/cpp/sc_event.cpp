/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_event.hpp"
#include "sc_utils.hpp"
#include "sc_addr.hpp"
#include "sc_memory.hpp"

#include "utils/sc_log.hpp"

// Should be equal to C values
ScEvent::Type ScEvent::AddOutputEdge = SC_EVENT_ADD_OUTPUT_ARC;
ScEvent::Type ScEvent::AddInputEdge = SC_EVENT_ADD_INPUT_ARC;
ScEvent::Type ScEvent::RemoveOutputEdge = SC_EVENT_REMOVE_OUTPUT_ARC;
ScEvent::Type ScEvent::RemoveInputEdge = SC_EVENT_REMOVE_INPUT_ARC;
ScEvent::Type ScEvent::EraseElement = SC_EVENT_REMOVE_ELEMENT;
ScEvent::Type ScEvent::ContentChanged = SC_EVENT_CONTENT_CHANGED;

ScEvent::ScEvent(const ScMemoryContext & ctx, const ScAddr & addr, Type eventType, ScEvent::DelegateFunc func /*= DelegateFunc()*/)
{
	mDelegate = func;
	mEvent = sc_event_new_ex(*ctx, *addr, (sc_event_type)eventType, (sc_pointer)this, &ScEvent::_handler, &ScEvent::_handlerDelete);
}

ScEvent::~ScEvent()
{
    mLock.lock();
	if (mEvent)
		sc_event_destroy(mEvent);
    mLock.unlock();
}

void ScEvent::removeDelegate()
{
	mDelegate = DelegateFunc();
}

sc_result ScEvent::_handler(sc_event const * evt, sc_addr edge, sc_addr other_el)
{
	ScEvent * eventObj = (ScEvent*)sc_event_get_data(evt);
	check_expr(eventObj != nullptr);

	if (eventObj->mDelegate)
	{
		return eventObj->mDelegate(ScAddr(sc_event_get_element(evt)), ScAddr(edge), ScAddr(other_el)) ? SC_RESULT_OK : SC_RESULT_ERROR;
	}

	return SC_RESULT_ERROR;
}

sc_result ScEvent::_handlerDelete(sc_event const * evt)
{
	ScEvent * eventObj = (ScEvent*)sc_event_get_data(evt);
	check_expr(eventObj != nullptr);

    eventObj->mLock.lock();
	if (eventObj->mEvent)
	{
		eventObj->mEvent = nullptr;
	}
    eventObj->mLock.unlock();

	return SC_RESULT_OK;
}