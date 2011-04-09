
#include "pp_Revision.h"
#include "ODi_ListenerState.h"
#include "ODi_StartTag.h"
#include "ODi_ElementStack.h"
#include <ut_conversion.h>
#include "ODi_Abi_ChangeTrackingRevisionMapping.h"

UT_uint32
ODi_ListenerState::getImplicitRemovalVersion()
{
    if( const ODi_StartTag* st = m_rElementStack.getClosestElement( "delta:intermediate-content" ))
    {
        st = m_rElementStack.getClosestElement( "delta:merge" );
        if( st )
        {
            if( const char* v = st->getAttributeValue( "delta:removal-change-idref" ))
            {
                return fromChangeID( v );
            }
        }
    }
    return 0;
}

/**
 * If the element is inside a delta:merge or a delta:removed-content
 * then change the passed revision attribute to include a REVISION_DELETION
 * at the right revision
 */
void
ODi_ListenerState::updateToHandleRemovalVersion( PP_RevisionAttr& ra )
{
    const gchar ** pAttrs = 0;
    const gchar ** pProps = 0;

    if( UT_uint32 v = getImplicitRemovalVersion() )
    {
        UT_DEBUGMSG(("updateToHandleRemovalVersion() implicit del revision:%d\n",v));
        ra.addRevision( v, PP_REVISION_DELETION, pAttrs, pProps );
    }
    else if( const ODi_StartTag* st = m_rElementStack.getClosestElement( "delta:removed-content" ))
    {
        if( const char* ver = st->getAttributeValue( "delta:removal-change-idref" ))
        {
            ra.addRevision( fromChangeID(ver),
                            PP_REVISION_DELETION,
                            pAttrs, pProps );
        }
    }
}


UT_uint32
ODi_ListenerState::fromChangeID( const std::string s )
{
    if( m_pAbiCTMap )
        return m_pAbiCTMap->getMapping( s );
    UT_DEBUGMSG(("BAD: fromChangeID() called without a change-id to abiword revision mapping!\n" ));
    return toType<UT_uint32>( s );
}

