#include "common.h"

int lg2_merge_base(git_repository* repo, int argc, char** argv)
{
	git_oid base;
	git_revspec revspec1;
	git_revspec revspec2;

	char str[GIT_OID_HEXSZ + 1];

	int err;

	err = git_revparse(&revspec1, repo, argv[argc - 1]);
	if (err != 0)
	{
		fprintf(stderr, "failed to lookup rev: %s\n", git_error_last()->message);
		goto cleanup;
	}

	err = git_revparse(&revspec2, repo, argv[argc - 2]);
	if (err != 0)
	{
		fprintf(stderr, "failed to lookup rev: %s\n", git_error_last()->message);
		goto cleanup;
	}

	err = git_merge_base(&base, repo, revspec1.from, revspec2.from);

	if (err != 0)
	{
		fprintf(stderr, "merge base error: %s\n", git_error_last()->message);
		goto cleanup;
	}

	git_oid_tostr(str, sizeof(str), &base);
	printf("%s\n", str);
cleanup:
	return 0;
}
