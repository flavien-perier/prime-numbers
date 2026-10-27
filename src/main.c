#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <argp.h>
#include <string.h>
#include <signal.h>

#include "write.h"
#include "primeNumbers.h"

const char *argp_program_version = "Prime Numbers 1.0.0";
const char *argp_program_bug_address = "<perier@flavien.io>";
static char doc[] = "Multithreaded prime number calculator.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {.name = "help", .key = 'h', .arg = NULL, .doc = "Show this message.", .group = -1},
    {.name = "rank", .key = 'r', .arg = "NUMBER", .doc = "Give the prime numbers up to rank."},
    {.name = "json", .key = 'j', .doc = "Use json format."},
    {.name = "output", .key = 'o', .arg = "FILE", .doc = "Output file."},
    {0}
};

struct arguments {
    unsigned long long int rank;
    bool json;
    char *output;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    char *endPtr = NULL;

    switch (key) {
        case 'h':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;
        case 'j':
            arguments->json = true;
            break;
        case 'r':
            if (arg == NULL || *arg == '\0' || *arg == '-') {
                argp_error(state, "the rank must be a positive number");
            }
            errno = 0;
            arguments->rank = strtoull(arg, &endPtr, 10);
            if (errno == ERANGE || endPtr == arg || *endPtr != '\0') {
                argp_error(state, "invalid number for the rank: '%s'", arg);
            }
            break;
        case 'o':
            arguments->output = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {.options = options, .parser = parse_opt, .args_doc = args_doc, .doc = doc};

static void signalHandler(int sig) {
    (void) sig;
    stopPrimeNumbers();
}

int main(int argc, char *argv[]) {
    struct arguments arguments = {.rank = 0, .json = false, .output = NULL};

    argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &arguments);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);

    if (arguments.output != NULL) {
        openOutputFile(&PRIME_NUMBER_OUTPUT_FILE, arguments.output);
    }

    if (arguments.json) {
        writeOutput("[");
    }

    unsigned long long int count = 0;
    mpz_t *primes = primeNumbers(arguments.rank, true, arguments.json, &count);

    if (arguments.json) {
        writeOutput("]\n");
    } else {
        writeOutput("\n");
    }

    closeOutputFile();
    freePrimeNumbers(primes, count);

    return EXIT_SUCCESS;
}