// Include other headers.
#include "src/exceptions/BaseException.h"
#include "src/utility/macros.h"
#include "src/cli/cli.h"
#include "src/utility/initialize.h"
#include "src/utility/Stopwatch.h"

#include "src/settings/SettingsManager.h"

/*!
 * Main entry point of the executable storm.
 */
int main(const int argc, const char** argv) {

    try {
        storm::utility::Stopwatch runtimeStopWatch;
        
        storm::utility::setUp();
        storm::cli::printHeader("Storm", argc, argv);
        storm::settings::initializeAll("Storm", "storm");
        bool optionsCorrect = storm::cli::parseOptions(argc, argv);
        if (!optionsCorrect) {
            return -1;
        }
        
        // From this point on we are ready to carry out the actual computations.
        storm::cli::processOptions();
        
        // All operations have now been performed, so we clean up everything and terminate.
        storm::utility::cleanUp();
        
        if(storm::settings::getModule<storm::settings::modules::GeneralSettings>().isPrintTimingsSet()) {
            std::cout << "Overall runtime: " << runtimeStopWatch << " seconds." << std::endl;
        }
        
        return 0;
    } catch (storm::exceptions::BaseException const& exception) {
        STORM_LOG_ERROR("An exception caused Storm to terminate. The message of the exception is: " << exception.what());
    } catch (std::exception const& exception) {
        STORM_LOG_ERROR("An unexpected exception occurred and caused Storm to terminate. The message of this exception is: " << exception.what());
    }
}
