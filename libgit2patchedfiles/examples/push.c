/*
 * libgit2 "push" example - shows how to push to remote
 *
 * Written by the libgit2 contributors
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "common.h"

struct push_opts {
	char* refspec;
};
static int push_update_reference_callback(const char* refname, const char* status, void* data);
static int sideband_progress_callback(const char* str, int len, void* payload);
static void parse_opts(struct push_opts* o, int argc, char* argv[]);

static void print_usage(void)
{
	fprintf(stderr, "usage:\n"
		"push (without arguments pushes to the current HEAD)\n"
		"push <refspec>\n");
	exit(1);
}
/**
 * This example demonstrates the libgit2 push API to roughly
 * simulate `git push`.
 */

 /** Entry point for this command */
int lg2_push(git_repository* repo, int argc, char** argv) {
	struct push_opts opt;
	git_push_options options;
	git_remote* remote = NULL;
	char* refspec = NULL;
	git_reference* head_ref = NULL;

	parse_opts(&opt, argc, argv);

	if (opt.refspec)
	{
		refspec = opt.refspec;
	}
	else
	{
		git_reference_lookup(&head_ref, repo, "HEAD");
		refspec = git_reference_symbolic_target(head_ref);
	}

	const git_strarray refspecs = {
		&refspec,
		1
	};
	check_lg2(git_remote_lookup(&remote, repo, "origin"), "Unable to lookup remote", NULL);

	check_lg2(git_push_options_init(&options, GIT_PUSH_OPTIONS_VERSION), "Error initializing push", NULL);
	options.callbacks.push_update_reference = push_update_reference_callback;
	options.callbacks.sideband_progress = sideband_progress_callback;

	check_lg2(git_remote_push(remote, &refspecs, &options), "Error pushing", NULL);
	printf("Successfully pushed to %s\n", refspec);

	git_reference_free(head_ref);

	return 0;
}


static int sideband_progress_callback(const char* str, int len, void* payload)
{
    if (strstr(str, "ERR") || strstr(str, "403") || strstr(str, "422"))
    {
	    fprintf(stderr, "%s\n", str);
    }
    
    return 0;
}

static int push_update_reference_callback(const char* refname, const char* status, void* data)
{
    if (status)
    {
	    fprintf(stderr, "%s\n", status);
	    exit(1);
    }

    return 0;
}

/**
 * Parse options that git's push command supports.
 */
static void parse_opts(struct push_opts* opt, int argc, char* argv[])
{
	memset(opt, 0, sizeof(*opt));

	if (argc == 1)
		return;

	if (argc > 2)
		print_usage();

	opt->refspec = argv[1];
}
