#include "gtest/gtest.h"

#include "RialtoTest.hpp"

// run this from the root dir of the rialto-geopackage repo
// you will need to set tempdir and datadir below

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    {
        using namespace pdal;
        using namespace rialto;
        using namespace rialtotest;

        Support::tempdir = "/tmp/rialtotests";
        if (!FileUtils::directoryExists(Support::tempdir))
        {
            FileUtils::createDirectory(Support::tempdir);
        }
        
        Support::datadir = "../rialto-data/demo";
        if (!FileUtils::directoryExists(Support::datadir))
        {
            fprintf(stderr, "datadir does not exist: %s\n", Support::datadir.c_str());
            exit(1);
        }
        
        setenv("RIALTO_HEARTBEAT", "false", true);
        setenv("RIALTO_EVENTS", "false", true);
    }
    
    return RUN_ALL_TESTS();
}
