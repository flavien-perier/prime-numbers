#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <signal.h>

#include "write.h"
#include "primeNumbers.h"

FILE* PRIME_NUMBER_OUTPUT_FILE = NULL;

const char *argp_program_version = "Prime Numbers 1.0.0";
const char *argp_program_bug_address = "<perier@flavien.io>";
static char doc[] = "Multithreaded prime number calculator.";
static char args_doc[] = "";

static struct argp_option options[] = {
        { "help", 'h', 0, 0, "Show this message.", -1 },
        {"rank",  'r', "NUMBER",      0,  "Compare words instead of characters." },
        {"json",    'j', 0,      0,  "Use json format." },
        {"output",    'o', "FILE",      0,  "Output file." },
        { 0 }
};

struct arguments {
    unsigned long long int rank;
    unsigned char json;
    char* output;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    char* endPtr;

    switch (key) {
        case 'h':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;
        case 'j':
            arguments->json = 1;
            break;
        case 'r':
            arguments->rank = strtoull(arg, &endPtr, 10);
            break;
        case 'o':
            arguments->output = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void signalHandler(int signal) {
    stopPrimeNumbers();
}

int main(int argc, char *argv[]) {
    struct arguments arguments;
    arguments.rank = 0;
    arguments.json = 0;
    arguments.output = "";

    argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &arguments);

    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);

    if (strcmp(arguments.output, "") != 0) {
        openOutputFile(&PRIME_NUMBER_OUTPUT_FILE, arguments.output);
    }

    if (arguments.json) {
        write("[");
    }

    primeNumbers(arguments.rank, 1, arguments.json);

    if (arguments.json) {
        write("]\n");
    }

    closeOutputFile();
    exit(EXIT_SUCCESS);
}
