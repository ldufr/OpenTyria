#pragma once

static const char *addresses[] = {
    "127.0.0.1:6112",
    "127.0.0.1:6113",
};

bool volatile keep_running = true;

static void signal_handler(int signum)
{
    UNREFERENCED_PARAMETER(signum);
    keep_running = false;
}

int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);

    int ret;
    if ((ret = sys_init()) != 0) {
        return 1;
    }

    for (int idx = 1; idx < argc; ++idx) {
        log_info("Param %d is: '%s'", idx, argv[idx]);
    }

    mbedtls_mpi_init(&prime_modulus);
    mbedtls_mpi_init(&server_private);

    const char *prime_modulus_ascii = "11369030006184639557951841839086711170523356038756186113745357553615433746914467378379225197818567184257516325746258410038643664166668140728650714232881773";
    if ((ret = mbedtls_mpi_read_string(&prime_modulus, 10, prime_modulus_ascii)) != 0) {
        log_error("failed to read the prime modulus '%s'", prime_modulus_ascii);
        return 1;
    }

    const char *server_private_ascii = "4955013838800442477563322845022960998436564559618750588243916809877035796638982560199828129813551335307191644041932374697066251319749593910087044611412991";
    if ((ret = mbedtls_mpi_read_string(&server_private, 10, server_private_ascii)) != 0) {
        log_error("failed to read the server private '%s'", server_private_ascii);
        return 1;
    }

    AuthSrv server;
    if (AuthSrv_Setup(&server) != 0) {
        log_error("Failed to initialize a server");
        return 1;
    }

    for (size_t idx = 0; idx < ARRAY_SIZE(addresses); ++idx) {
        const char *addr = addresses[idx];
        if (AuthSrv_Bind(&server, addr, strlen(addr)) != ERR_OK) {
            log_error("Failed to bind the address '%s'", addr);
            return 1;
        }
    }

    const char addr[] = "127.0.0.1:1337";
    if (AuthSrv_Bind(&server, addr, sizeof(addr) - 1) != ERR_OK) {
        log_error("Failed to bind the internal address '%s'", addr);
        return 1;
    }

    if (!parse_addr(&server.internal_address, addr, sizeof(addr) - 1)) {
        log_error("Failed to parse the internal address '%s'", addr);
        return 1;
    }

    while (keep_running) {
        AuthSrv_Update(&server);
    }

    if ((ret = sys_free()) != 0) {
        log_warn("Didn't unitialize correctly (ret: %d)", ret);
    }

    return 0;
}
