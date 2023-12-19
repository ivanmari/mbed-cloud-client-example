// ----------------------------------------------------------------------------
// Copyright 2021 Pelion Ltd.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------
#include <stdint.h>
#include <inttypes.h>

#define TRACE_GROUP "FOTA"

#include "fota/fota_app_ifs.h"    // required for implementing custom install callback for Linux like targets
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



#define ACTIVATE_SCRIPT_LENGTH 512

//PATCH_BEG UPG LED
#define UPGRADE_LED_NAME led_08
#define BLINK_PERIOD_DOWNLOAD_UPGRADE 1
//PATCH_END

#if !(defined (FOTA_DEFAULT_APP_IFS) && FOTA_DEFAULT_APP_IFS==1)
int fota_app_on_complete(int32_t status)
{
    if (status == FOTA_STATUS_SUCCESS) {
        printf("FOTA was successfully completed\n");
    } else {
        printf("FOTA failed with status %" PRId32 "\n", status);
    }

    return FOTA_STATUS_SUCCESS;
}

void fota_app_on_download_progress(size_t downloaded_size, size_t current_chunk_size, size_t total_size)
{
    static const uint32_t  print_range_percent = 5;
    uint32_t progress = (downloaded_size + current_chunk_size) * 100 / total_size;
    uint32_t prev_progress = downloaded_size * 100 / total_size;

    if (downloaded_size == 0 || ((progress / print_range_percent) > (prev_progress / print_range_percent))) {
        printf("Downloading firmware. %" PRIu32 "%%\n", progress);
    }
}

int fota_app_on_install_authorization()
{
    printf("Firmware install authorized\n");
    fota_app_authorize();
    return FOTA_STATUS_SUCCESS;
}

int  fota_app_on_download_authorization(
    const manifest_firmware_info_t *candidate_info,
    fota_component_version_t curr_fw_version)
{
    char curr_semver[FOTA_COMPONENT_MAX_SEMVER_STR_SIZE] = { 0 };
    char new_semver[FOTA_COMPONENT_MAX_SEMVER_STR_SIZE] = { 0 };

    fota_component_version_int_to_semver(curr_fw_version, curr_semver);
    fota_component_version_int_to_semver(candidate_info->version, new_semver);

    printf("Firmware download requested (priority=%" PRIu32 ")\n", candidate_info->priority);
    printf(
        "Updating component %s from version %s to %s\n",
        candidate_info->component_name,
        curr_semver,
        new_semver
    );

    printf("Update priority %" PRIu32 "\n", candidate_info->priority);

    if (candidate_info->payload_format == FOTA_MANIFEST_PAYLOAD_FORMAT_DELTA) {
        printf(
            "Delta update. Patch size %zuB full image size %zuB\n",
            candidate_info->payload_size,
            candidate_info->installed_size
        );
    } else {
        printf("Update size %zuB\n", candidate_info->payload_size);
    }

    fota_app_authorize();


    //PATCH_BEGING UPG LED
    int rc;
    char command[ACTIVATE_SCRIPT_LENGTH] = {0};
                
    int length = snprintf(command,
                          ACTIVATE_SCRIPT_LENGTH,
                          "%s %s %s",
                          " busctl call jci.obbas.hal /jci/hal/Led jci.hal.Led LedBlink st", UPGRADE_LED_NAME, BLINK_PERIOD_DOWNLOAD_UPGRADE);

    FOTA_TRACE_INFO( "shell command from fota install calback %s", command );
                                                                                                              
    /* execute script command */
    rc = system(command);
    if( rc ) {
        ret = FOTA_STATUS_FW_INSTALLATION_FAILED;
        if( rc == -1 ) {        
            FOTA_TRACE_ERROR( "shell could not be run" );
        } else {
            FOTA_TRACE_ERROR( "result of running command is %d", WEXITSTATUS(rc) );
        }
    }
    return ret;
    //PATCH_END



    return FOTA_STATUS_SUCCESS;
}
#endif //#if !(defined (FOTA_DEFAULT_APP_IFS) && FOTA_DEFAULT_APP_IFS==1)

// e.g. Yocto target have different update activation logic residing outside of the example
// Simplified Linux use case example.
// For MAIN component update the the binary file current process is running.
// Simulate component update by just printing its name.
// After the installation callback returns, FOTA will "reboot" by calling pal_osReboot().

int fota_app_on_install_candidate(const char *candidate_fs_name, const manifest_firmware_info_t *firmware_info)
{
    int ret = FOTA_STATUS_SUCCESS;
    int rc;
    char command[ACTIVATE_SCRIPT_LENGTH] = {0};
                
    int length = snprintf(command,
                          ACTIVATE_SCRIPT_LENGTH,
                          "%s %s %s",
                          "./fota_update_activate.sh",  candidate_fs_name, MBED_CLOUD_CLIENT_FOTA_LINUX_HEADER_FILENAME);
    FOTA_ASSERT(length < ACTIVATE_SCRIPT_LENGTH);

    FOTA_TRACE_INFO( "shell command from fota install calback %s", command );
                                                                                                              
    /* execute script command */
    rc = system(command);
    if( rc ) {
        ret = FOTA_STATUS_FW_INSTALLATION_FAILED;
        if( rc == -1 ) {        
            FOTA_TRACE_ERROR( "shell could not be run" );
        } else {
            FOTA_TRACE_ERROR( "result of running command is %d", WEXITSTATUS(rc) );
        }
    }
    return ret;
}


int fota_app_on_main_app_verify_install(const fota_header_info_t *expected_header_info)
{
    int ret = FOTA_STATUS_SUCCESS;
    return ret;
}
