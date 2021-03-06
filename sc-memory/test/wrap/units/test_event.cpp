/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include "../test.hpp"
#include "sc-memory/cpp/sc_event.hpp"
#include "sc-memory/cpp/sc_timer.hpp"
#include "sc-memory/cpp/sc_stream.hpp"

#include <thread>
#include <mutex>

namespace
{
    const double kTestTimeout = 5.0;
    
    template<typename EventClassT, typename PrepareF, typename EmitF>
    void testEventsFuncT(ScMemoryContext & ctx, ScAddr const & addr, PrepareF prepare, EmitF emit)
    {
        prepare();

        volatile bool isDone = false;
        auto const callback = [&isDone](ScAddr const & listenAddr,
                                        ScAddr const & edgeAddr,
                                        ScAddr const & otherAddr)
        {
            isDone = true;
            return true;
        };

        EventClassT evt(ctx, addr, callback);
        ScTimer timer(kTestTimeout);

        emit();

        while (!isDone && !timer.IsTimeOut())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        SC_CHECK(isDone, ());
    }
}



UNIT_TEST(events_common)
{
	ScMemoryContext ctx(sc_access_lvl_make_min, "events_common");

    SUBTEST_START(ScEventAddInputEdge)
    {
        ScAddr addr;
        auto const createNode = [&ctx, &addr]()
        {
            addr = ctx.createNode(0);
            SC_CHECK(addr.isValid(), ());
        };

        auto const emitEvent = [&ctx, &addr]()
        {
            ScAddr const addr2 = ctx.createNode(0);
            SC_CHECK(addr2.isValid(), ());

            ScAddr const edge = ctx.createEdge(*ScType::EDGE_ACCESS, addr2, addr);
            SC_CHECK(edge.isValid(), ());
        };

        testEventsFuncT<ScEventAddInputEdge>(ctx, addr, createNode, emitEvent);
    }
    SUBTEST_END

    SUBTEST_START(ScEventAddOutputEdge)
    {
        ScAddr addr;
        auto const createNode = [&ctx, &addr]()
        {
            addr = ctx.createNode(0);
            SC_CHECK(addr.isValid(), ());
        };

        auto const emitEvent = [&ctx, &addr]()
        {
            ScAddr const addr2 = ctx.createNode(0);
            SC_CHECK(addr2.isValid(), ());

            ScAddr const edge = ctx.createEdge(*ScType::EDGE_ACCESS, addr, addr2);
            SC_CHECK(edge.isValid(), ());
        };

        testEventsFuncT<ScEventAddOutputEdge>(ctx, addr, createNode, emitEvent);
    }
    SUBTEST_END;

    SUBTEST_START(ScEventRemoveInputEdge)
    {
        ScAddr const addr = ctx.createNode(0);
        SC_CHECK(addr.isValid(), ());

        ScAddr const addr2 = ctx.createNode(0);
        SC_CHECK(addr2.isValid(), ());

        ScAddr const edge = ctx.createEdge(*ScType::EDGE_ACCESS, addr, addr2);
        SC_CHECK(edge.isValid(), ());

        auto const prepare = []() {};
        auto const emitEvent = [&ctx, &edge]()
        {
            SC_CHECK(ctx.eraseElement(edge), ());
        };

        testEventsFuncT<ScEventRemoveInputEdge>(ctx, addr2, prepare, emitEvent);
    }
    SUBTEST_END

    SUBTEST_START(ScEventRemoveOutputEdge)
    {
        ScAddr const addr = ctx.createNode(0);
        SC_CHECK(addr.isValid(), ());

        ScAddr const addr2 = ctx.createNode(0);
        SC_CHECK(addr2.isValid(), ());

        ScAddr const edge = ctx.createEdge(*ScType::EDGE_ACCESS, addr, addr2);
        SC_CHECK(edge.isValid(), ());

        auto const prepare = []() {};
        auto const emitEvent = [&ctx, &edge]()
        {
            SC_CHECK(ctx.eraseElement(edge), ());
        };

        testEventsFuncT<ScEventRemoveOutputEdge>(ctx, addr, prepare, emitEvent);
    }
    SUBTEST_END

    SUBTEST_START(ScEventContentChanged)
    {
        ScAddr const addr = ctx.createLink();
        SC_CHECK(addr.isValid(), ());

        auto const prepare = []() {};
        auto const emitEvent = [&ctx, &addr]()
        {
            std::string const value("test");
            ScStream stream((sc_char*)value.data(), static_cast<sc_uint32>(value.size()), SC_STREAM_FLAG_READ);
            SC_CHECK(ctx.setLinkContent(addr, stream), ());
        };

        testEventsFuncT<ScEventContentChanged>(ctx, addr, prepare, emitEvent);
    }
    SUBTEST_END

    SUBTEST_START(ScEventEraseElement)
    {
        ScAddr const addr = ctx.createNode(0);
        SC_CHECK(addr.isValid(), ());

        auto const prepare = []() {};
        auto const emitEvent = [&ctx, &addr]()
        {
            SC_CHECK(ctx.eraseElement(addr), ());
        };

        testEventsFuncT<ScEventEraseElement>(ctx, addr, prepare, emitEvent);
    }
    SUBTEST_END
}


UNIT_TEST(events_destroy_order)
{
    ScMemoryContext * ctx = new ScMemoryContext(sc_access_lvl_make_min, "events_destroy_order");

    ScAddr const node = ctx->createNode(0);
    SC_CHECK(node.isValid(), ());

    ScEventAddOutputEdge * evt = new ScEventAddOutputEdge(*ctx, node,
        [](ScAddr const &, ScAddr const &, ScAddr const &)
    {
        return true;
    });

    delete ctx;
    
    // delete event after context
    delete evt;
}
