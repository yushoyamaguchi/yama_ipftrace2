/*
 * SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
 * Copyright (C) 2020 Yutaro Hayakawa
 */
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipftrace.h"

static char *default_output_type = "aggregate";

static struct option options[] = {
    {"help", no_argument, 0, 'h'},
    {"list", no_argument, 0, 'l'},
    {"mark", required_argument, 0, 'm'},
    {"output", required_argument, 0, 'o'},
    {"regex", required_argument, 0, 'r'},
    {"mask", required_argument, 0, '0'},
    {"perf-page-count", required_argument, 0, '0'},
    {"test", no_argument, 0, '0'},
    {"no-set-rlimit", no_argument, 0, '0'},
    {NULL, 0, 0, 0},
};

static void
usage(void)
{
  fprintf(stderr,
          "Usage: ipft [OPTIONS]\n"
          "\n"
          "Options:\n"
          " -h, --help                            Show this text\n"
          " -l, --list                            List functions\n"
          " -m, --mark            [HEX]           Trace the packet "
          "marked with <mark> [required]\n"
          "   , --mask            [HEX]           Only match to the bits masked "
          "with given bitmask\n"
          " -o, --output          [OUTPUT-FORMAT] Specify output format\n"
          " -r, --regex           [REGEX]         Filter the function to trace"
          "with regex\n"
          "   , --perf-page-count [NUMBER]        Number of pages to use with"
          "perf\n"
          "   , --test                            Run in eBPF test mode\n"
          "   , --no-set-rlimit                   Don't set rlimit\n"
          "\n"
          "DEBUG-FORMAT := { dwarf, btf }\n"
          "OUTPUT-FORMAT := { aggregate, stream }\n"
          "\n");
}

static void
opt_init(struct ipft_tracer_opt *opt)
{
  opt->mark = 0;
  opt->mask = 0xffffffff;
  opt->output_type = default_output_type;
  opt->perf_page_cnt = 8;
  opt->regex = NULL;
  opt->set_rlimit = true;
}

static void
opt_deinit(struct ipft_tracer_opt *opt)
{
  /* Compare address */
  if (opt->debug_info_type != default_debuginfo_type) {
    free(opt->debug_info_type);
  }

  if (opt->output_type != default_output_type) {
    free(opt->output_type);
  }

  if (opt->script_path != NULL) {
    free(opt->script_path);
  }

  if (opt->regex != NULL) {
    free(opt->regex);
  }
}

static bool
opt_validate(struct ipft_tracer_opt *opt, bool list)
{
  if (!list && opt->mark == 0) {
    fprintf(stderr, "-m --mark is missing (or specified 0 which is invalid)\n");
    return false;
  }

  if (!list && opt->mask == 0) {
    fprintf(stderr, "Masking by 0 is not allowed\n");
    return false;
  }

  if (strcmp(opt->output_type, "aggregate") != 0 &&
      strcmp(opt->output_type, "stream") != 0) {
    fprintf(stderr, "Invalid output format %s\n", opt->output_type);
    return false;
  }

  if (!list && opt->perf_page_cnt == 0) {
    fprintf(stderr, "Perf page count should be at least 1\n");
    return false;
  }

  return true;
}

int
main(int argc, char **argv)
{
  int c, optind;
  int error = -1;
  const char *optname;
  struct ipft_tracer_opt opt;
  bool list = false, test = false;

  opt_init(&opt);

  while ((c = getopt_long(argc, argv, "f:hlm:o:r:s:", options, &optind)) != -1) {
    switch (c) {
    case 'f':
      opt.debug_info_type = strdup(optarg);
      break;
    case 'l':
      list = true;
      break;
    case 'm':
      opt.mark = strtoul(optarg, NULL, 16);
      break;
    case 'o':
      opt.output_type = strdup(optarg);
      break;
    case 'r':
      opt.regex = strdup(optarg);
      break;
    case '0':
      optname = options[optind].name;

      if (strcmp(optname, "mask") == 0) {
        opt.mask = strtoul(optarg, NULL, 16);
        break;
      }

      if (strcmp(optname, "perf-page-count") == 0) {
        opt.perf_page_cnt = atoi(optarg);
        break;
      }

      if (strcmp(optname, "test") == 0) {
        test = true;
        break;
      }

      if (strcmp(optname, "no-set-rlimit") == 0) {
        opt.set_rlimit = false;
        break;
      }

      break;
    default:
      usage();
      goto end;
    }
  }

  if (!opt_validate(&opt, list)) {
    usage();
    goto end;
  }

  if (list) {
    error = list_functions(&opt);
    goto end;
  }

  if (test) {
    error = test_bpf_prog(&opt);
    goto end;
  }

  error = tracer_run(&opt);
  if (error == -1) {
    fprintf(stderr, "Trace failed with error\n");
  }

end:
  opt_deinit(&opt);
  return error == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
