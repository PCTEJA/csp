#ifndef _IN_CSP_ENGINE_DYNAMIC_H
#define _IN_CSP_ENGINE_DYNAMIC_H

#include <csp/engine/CspType.h>
#include <csp/engine/CycleStepTable.h>
#include <csp/engine/Node.h>
#include <functional>
#include <string>

namespace csp
{

class DynamicEngine;

//DynamicNode Is an instance of an outter node that is responsible for a single csp.dynamic() call in the graph
//Internally it will maintain N DynamicEngines, 1 per instantiated key.  It is responsible for reacting to all
//external inputs and triggering a cycle step.  
//All dynamic engine input adapters as well as all external timeseries wired into dynamic are registered as inputs
//into the DynanicNode instance so that it can invoke cycle step at the appropriate times.
class DynamicNode final : public Node
{
public:
    using Outputs = std::vector<const TimeSeriesProvider *>;
    using EngineBuilder = std::function<Outputs( DynamicNode *, DynamicEngine *, const DialectGenericType & key )>;

    DynamicNode( Engine * engine, const std::string & nodename, 
                 std::vector<INOUT_ID_TYPE> snapIds, 
                 EngineBuilder builder,
                 NodeDef def );
    ~DynamicNode();

    void executeImpl() override;
    void start() override;
    void stop() override;

    const char * name() const override { return m_name.c_str(); }

    void handleEvent( InputId id ) override;

    int64_t elemId( const DialectGenericType & key );

private:
    using InstanceMap = std::unordered_map<DialectGenericType, std::unique_ptr<DynamicEngine>>;

    void addDynamicInstance( const DialectGenericType & key );
    void removeDynamicInstance( const DialectGenericType & key );

    void handleTriggerShapeChange( const DialectGenericType & key, bool added, int64_t elemId, int64_t replaceId );

    //All instance engines share a single CycleStepTable on the DynamicNode
    CycleStepTable m_cycleStepTable;
    InstanceMap    m_instanceMap;
    EngineBuilder  m_engineBuilder;
    std::string    m_name;

    //set of keys requesting dynamic shutdown via csp.stop_engine calls
    std::unordered_set<DialectGenericType> m_dynamicShutdowns;

    std::vector<INOUT_ID_TYPE> m_snapIds; //temporary storage to make them passive

    //Mirror of the trigger input basket's key -> elemId mapping, maintained from
    //the basket's own change callback (see the constructor).  This is the mapping
    //elemId() resolves against for csp.attach() wiring: unlike m_dynamicKeyMap
    //below it follows the order shape events are applied to the trigger basket,
    //so it stays correct when a removal and an addition land in the same cycle.
    std::unordered_map<DialectGenericType,INOUT_ELEMID_TYPE> m_triggerKeyMap;
    std::vector<DialectGenericType>                          m_triggerElemIdMap;

    //We may be able to remove this once we move key management into DynamicBasketInfo
    //Mirror of the dynamic OUTPUT basket's key -> elemId mapping.  Keys are added
    //here when a dynamic instance is created and removed when the instance is
    //destroyed after the cycle (see executeImpl), so this stays in lockstep with
    //the output basket and provides the elemId removeDynamicInstance passes to it.
    //It does NOT track the trigger input basket, which applies removals eagerly.
    std::unordered_map<DialectGenericType,INOUT_ELEMID_TYPE> m_dynamicKeyMap;
    std::vector<DialectGenericType>                          m_dynamicElemIdMap;
};

}

#endif
