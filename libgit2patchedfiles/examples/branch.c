/*
 * libgit2 "branch" example - shows simplistic version of branch APIs
 */

#include "common.h"

struct branch_opts {
	int move;
	int delete;
	char* branch1;
	char* branch2;
};

static void parse_opts(struct branch_opts* o, int argc, char* argv[]);

static void print_usage(void)
{
	fprintf(stderr, "usage:\n"
		"branch -m <oldbranch> <newbranch>\n"
		"branch -d <branch>\n");
	exit(1);
}

/*
 * git branch -m [<oldbranch>] <newbranch>
 * git branch -d <branchname>
 */
int lg2_branch(git_repository* repo, int argc, char* argv[])
{
	struct branch_opts opt;
	git_reference* branch1 = NULL;
	git_reference* new_ref = NULL;

	parse_opts(&opt, argc, argv);

	if (opt.branch1)
	{
		check_lg2(git_branch_lookup(&branch1, repo, opt.branch1, GIT_BRANCH_LOCAL), "Error branch lookup", NULL);
	}

	if (opt.delete && branch1)
	{
		check_lg2(git_branch_delete(branch1), "Error branch delete", NULL);
		printf("%s branch was successfully deleted\n", git_reference_name(branch1));
	}

	if (opt.move && branch1 && opt.branch2)
	{
		check_lg2(git_branch_move(&new_ref, branch1, opt.branch2, 0), "Error branch move", NULL);
		printf("%s branch was successfully renamed to %s\n", opt.branch1, opt.branch2);
	}

	git_reference_free(branch1);
	git_reference_free(new_ref);

	return 0;
}

/**
 * Parse options that git's branch command supports.
 */
static void parse_opts(struct branch_opts* opt, int argc, char* argv[])
{
	struct args_info args = ARGS_INFO_INIT;

	if (argc <= 1)
		print_usage();

	memset(opt, 0, sizeof(*opt));

	for (args.pos = 1; args.pos < argc; ++args.pos) {
	char* a = argv[args.pos];

	if (a[0] != '-') {
		if (!opt->branch1)
			opt->branch1 = a;
		else if (!opt->branch2)
			opt->branch2 = a;
		else
			print_usage();
	}
	else if (!strcmp(a, "-m"))
		opt->move = 1;
	else if (!strcmp(a, "-d"))
		opt->delete = 1;
	else
		print_usage();
	}

	if (opt->move && !opt->branch2)
		print_usage();
}
