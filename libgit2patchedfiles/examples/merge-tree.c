/*
 * libgit2 "merge-tree" example
 */

#include "common.h"

struct merge_tree_opts {
    char* branch1;
    char* branch2;
};

static void parse_opts(struct merge_tree_opts* o, int argc, char* argv[]);

static void print_usage(void)
{
    fprintf(stderr, "usage:\n"
	"merge-tree <branch1> <branch2>");
    exit(1);
}

static void output_conflicts(git_index* index)
{
    git_index_conflict_iterator* conflicts;
    const git_index_entry* ancestor;
    const git_index_entry* our;
    const git_index_entry* their;
    int err = 0;

    check_lg2(git_index_conflict_iterator_new(&conflicts, index), "failed to create conflict iterator", NULL);

    while ((err = git_index_conflict_next(&ancestor, &our, &their, conflicts)) == 0) {
	printf("conflict: a:%s o:%s t:%s\n",
	    ancestor ? ancestor->path : "NULL",
	    our->path ? our->path : "NULL",
	    their->path ? their->path : "NULL");
    }

    if (err != GIT_ITEROVER) {
	fprintf(stderr, "error iterating conflicts\n");
    }

    git_index_conflict_iterator_free(conflicts);
}

/*
 * git merge-tree <branch1> <branch2>
 */
int lg2_merge_tree(git_repository* repo, int argc, char* argv[])
{
    struct merge_tree_opts opt;

    git_object* obj1 = NULL;
    git_object* obj2 = NULL;

    git_oid ancestor_oid;
    git_commit *our_commit = NULL;
    git_commit *their_commit = NULL;
    git_commit *ancestor_commit = NULL;
    git_tree *our_tree = NULL;
    git_tree* their_tree = NULL;
    git_tree *ancestor_tree = NULL;

    git_index *index = NULL;
    int error = 0;

    parse_opts(&opt, argc, argv);

    if (opt.branch1 && opt.branch2)
    {
	    if (
		    (error = git_revparse_single(&obj1, repo, opt.branch1)) < 0 ||
		    (error = git_revparse_single(&obj2, repo, opt.branch2)) < 0 ||
		    (error = git_merge_base(&ancestor_oid, repo, git_object_id(obj1), git_object_id(obj2))) < 0 ||

		    (error = git_commit_lookup(&our_commit, repo, git_object_id(obj1))) < 0 ||
		    (error = git_commit_lookup(&their_commit, repo, git_object_id(obj2))) < 0 ||
		    (error = git_commit_lookup(&ancestor_commit, repo, &ancestor_oid)) < 0 ||

		    (error = git_commit_tree(&ancestor_tree, ancestor_commit)) < 0 ||
		    (error = git_commit_tree(&our_tree, our_commit)) < 0 ||
		    (error = git_commit_tree(&their_tree, their_commit)) < 0 ||

		    (error = git_merge_trees(&index, repo, ancestor_tree, our_tree, their_tree, NULL)) < 0
		)
			    goto cleanup;

	    if (git_index_has_conflicts(index)) {
		    output_conflicts(index);
	    }
    }
    else
    {
	    print_usage();
    }


cleanup:
    git_index_free(index);
    git_tree_free(our_tree);
    git_tree_free(their_tree);
    git_tree_free(ancestor_tree);
    git_commit_free(our_commit);
    git_commit_free(their_commit);
    git_commit_free(ancestor_commit);
    git_object_free(obj1);
    git_object_free(obj2);

    return error;
}

/**
 * Parse options that git's merge-tree command supports.
 */
static void parse_opts(struct merge_tree_opts* opt, int argc, char* argv[])
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
	} else
	    print_usage();
    }
}
