typedef enum SubCommand {
    SubCommand_None,
    SubCommand_Toc,
    SubCommand_List,
    SubCommand_Extract,
    SubCommand_Find,
} SubCommand;

typedef struct CommandOptions {
    bool        print_help;
    bool        print_version;
    bool        verbose;
    bool        trace;
    SubCommand  subcommand;
    const char *archive;
    const char *extract_dir;
    struct {
        bool set;
        uint32_t val;
    } file_id;
} CommandOptions;

CommandOptions options;

SubCommand parse_subcommand(const char *arg)
{
    if (strcmp(arg, "toc") == 0) {
        return SubCommand_Toc;
    } else if (strcmp(arg, "list") == 0) {
        return SubCommand_List;
    } else if (strcmp(arg, "extract") == 0) {
        return SubCommand_Extract;
    } else if (strcmp(arg, "find") == 0) {
        return SubCommand_Find;
    } else {
        return SubCommand_None;
    }
}

#define PROGRAM_NAME "DatPack"

void print_help(FILE *stream, bool terminate)
{
    fprintf(
        stream,
        PROGRAM_NAME ": [--version] [--help] <archive> <command> [<args>]\n"

        "    --version                  print version and exist\n"
        "    -h, --help                 print this help\n"
        "    -v, --verbose              Enable debug logs\n"
        "    -vv, --trace               Enable trace logs\n"
        "\n\n"
        PROGRAM_NAME " toc"
        "\n\n"
        PROGRAM_NAME " list"
        "\n\n"
        PROGRAM_NAME " extract <extract dir>"
        "\n\n"
        PROGRAM_NAME " find <file id>"
        "\n\n"
    );

    if (terminate)
        exit(0);
}

void parse_next_position_arg(const char *arg)
{
    if (options.subcommand == SubCommand_Extract) {
        if (options.extract_dir == NULL) {
            options.extract_dir = arg;
            return;
        }
    }

    if (options.subcommand == SubCommand_Find) {
        if (!options.file_id.set) {
            options.file_id.set = true;
            options.file_id.val = (uint32_t) strtoul(arg, NULL, 0);
            if (!options.file_id.val && errno != 0) {
                fprintf(stderr, "Couldn't parse '%s' as file id\n", arg);
                exit(1);
            }
            return;
        }
    }

    fprintf(stderr, "Unknown argument '%s'\n", arg);
    exit(1);
}

void parse_command_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] == '-') {
            if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
                print_help(stdout, true);
            } else if (!strcmp(arg, "--version")) {
                options.print_version = true;
            } else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
                options.verbose = true;
            } else if (!strcmp(arg, "-vv") || !strcmp(arg, "--trace")) {
                options.trace = true;
            }
        } else if (options.archive == NULL) {
            options.archive = arg;
        } else if (options.subcommand == SubCommand_None) {
            if ((options.subcommand = parse_subcommand(arg)) == SubCommand_None) {
                fprintf(stderr, "Invalid subcommand '%s'\n", arg);
                exit(1);
            }
        } else {
            parse_next_position_arg(arg);
        }
    }
}
