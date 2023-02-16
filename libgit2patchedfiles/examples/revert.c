#include "common.h"

int lg2_revert(git_repository* repo, int argc, char** argv)
{
	git_commit* target_commit = NULL;
	git_revspec revspec;
	git_revert_options opts = GIT_REVERT_OPTIONS_INIT;
	int err;

	err = git_revparse(&revspec, repo, argv[argc - 1]);
	if (err != 0)
	{
		fprintf(stderr, "failed to lookup rev: %s\n", git_error_last()->message);
		goto cleanup;
	}
	err = git_commit_lookup(&target_commit, repo, revspec.from);
	if (err != 0)
	{
		fprintf(stderr, "failed to lookup commit: %s\n", git_error_last()->message);
		goto cleanup;
	}

	if (argc > 2)
	{
		if (!strcmp(argv[argc - 2], "-m"))
		{
			opts.mainline = 1;
		}
	}

	err = git_revert(repo, target_commit, &opts);
	if (err != 0)
	{
		fprintf(stderr, "revert error: %s\n", git_error_last()->message);
		goto cleanup;
	}
cleanup:
	git_commit_free(target_commit);

	return 0;
}
