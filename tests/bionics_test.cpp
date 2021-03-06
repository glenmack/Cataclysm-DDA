#include <climits>
#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "bionics.h"
#include "game.h"
#include "item.h"
#include "player.h"
#include "calendar.h"
#include "pimpl.h"
#include "string_id.h"
#include "type_id.h"

static void clear_bionics( player &p )
{
    p.my_bionics->clear();
    p.set_power_level( 0_kJ );
    p.set_max_power_level( 0_kJ );
}

static void give_and_activate( player &p, bionic_id const &bioid )
{
    INFO( "bionic " + bioid.str() + " is valid" );
    REQUIRE( bioid.is_valid() );

    p.add_bionic( bioid );
    INFO( "dummy has gotten " + bioid.str() + " bionic " );
    REQUIRE( p.has_bionic( bioid ) );

    // get bionic's index - might not be "last added" due to "integrated" ones
    int bioindex = -1;
    for( size_t i = 0; i < p.my_bionics->size(); i++ ) {
        const auto &bio = ( *p.my_bionics )[ i ];
        if( bio.id == bioid ) {
            bioindex = i;
        }
    }
    REQUIRE( bioindex != -1 );

    const bionic &bio = p.bionic_at_index( bioindex );
    REQUIRE( bio.id == bioid );

    // turn on if possible
    if( bio.id->toggled && !bio.powered ) {
        const std::vector<itype_id> fuel_opts = bio.info().fuel_opts;
        if( !fuel_opts.empty() ) {
            p.set_value( fuel_opts.front(), "2" );
        }
        p.activate_bionic( bioindex );
        INFO( "bionic " + bio.id.str() + " with index " + std::to_string( bioindex ) + " is active " );
        REQUIRE( p.has_active_bionic( bioid ) );
        if( !fuel_opts.empty() ) {
            p.remove_value( fuel_opts.front() );
        }
    }
}

static void test_consumable_charges( player &p, std::string &itemname, bool when_none,
                                     bool when_max )
{
    item it = item( itemname, 0, 0 ) ;

    INFO( "\'" + it.tname() + "\' is count-by-charges" );
    CHECK( it.count_by_charges() );

    it.charges = 0;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_none );

    it.charges = INT_MAX;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_max );
}

static void test_consumable_ammo( player &p, std::string &itemname, bool when_empty,
                                  bool when_full )
{
    item it = item( itemname, 0, 0 ) ;

    it.ammo_unset();
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_empty );

    it.ammo_set( it.ammo_default(), -1 ); // -1 -> full
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_full );
}

TEST_CASE( "bionics", "[bionics] [item]" )
{
    player &dummy = g->u;

    // one section failing shouldn't affect the rest
    clear_bionics( dummy );

    // Could be a SECTION, but prerequisite for many tests.
    INFO( "no power capacity at first" );
    CHECK( !dummy.has_max_power() );

    dummy.add_bionic( bionic_id( "bio_power_storage" ) );

    INFO( "adding Power Storage CBM only increases capacity" );
    CHECK( !dummy.has_power() );
    REQUIRE( dummy.has_max_power() );

    SECTION( "bio_advreactor" ) {
        give_and_activate( dummy, bionic_id( "bio_advreactor" ) );

        static const std::list<std::string> always = {
            "plut_cell",  // solid
            "plut_slurry" // uncontained liquid! not shown in game menu
        };
        for( auto it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> never = {
            "light_atomic_battery_cell", // TOOLMOD, no ammo actually
            "rm13_armor"      // TOOL_ARMOR
        };
        for( auto it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    SECTION( "bio_batteries" ) {
        give_and_activate( dummy, bionic_id( "bio_batteries" ) );

        static const std::list<std::string> always = {
            "battery" // old-school
        };
        for( auto it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> never = {
            "flashlight",  // !is_magazine()
            "laser_rifle", // NO_UNLOAD, uses ups_charges
            "UPS_off",     // NO_UNLOAD, !is_magazine()
            "battery_car"  // NO_UNLOAD, is_magazine()
        };
        for( auto it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}
