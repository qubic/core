# How to Contribute

## Contributing as an external developer

If you find bugs, typos, or other problems that can be fixed with a few changes, you are more than welcome to contribute these fixes with a pull request as follows.
For big changes, such as adding a smart contract, we recommend to discuss with one of the core developers first.

1. Create your own fork of this repository on GitHub. 
2. Clone your fork on your local machine. Your remote repository on GitHub is called origin.
3. Add the original GitHub repository as a remote called upstream.
4. If you have created your fork a while ago, pull upstream changes into your local repository.
5. Create a new branch from `develop`.
6. Fix the bug, correct the typo, solve the problem, etc. Make sure to follow the coding guidelines below.
7. Commit your changes to your new branch. Make sure to use a concise but meaningful commit message.
8. Push your branch to the remote origin (your fork on Github).
9. From your fork on GitHub, open a pull request (PR) in your new branch targeting the `develop` branch of the original repo. In the PR, describe the problem and how your changes solve it.
10. If the PR reviewer requests further changes, push them to your branch. The PR will be updated automatically.
11. When your PR is approved and merged, you can pull the changes from upstream to your local repo and delete your extra branch.


## Development workflow / branches

We use a mixture of [GitFlow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) and a [trunk-based-process](https://www.atlassian.com/continuous-delivery/continuous-integration/trunk-based-development) with the following branches:

1. `main`: The code base that is officially deployed. Further Releases are created from this branch.
Changes should NOT be committed here directly but merged from other branches after testing.
Exception: If the current release (such as v1.191.0) has a critical bug, the fix may be committed to the main branch directly (followed by creating a bugfix release such as v1.191.1 after testing the fix).

2. `develop`: Our current testing branch, which will be automatically deployed to the testnet once active.
Most changes should either be committed here or in feature branches.
Feature branches should be created from `develop` and pull requests (PR) should target `develop`.
Before committing to this branch or creating a PR, please check that everything compiles and the automated tests (gtest) run successfully.

3. `feature/YYYY-MM-DD-FeatureName`: When we are working on a specific feature that is quite complex and/or requires code review, a feature branch should be created from `develop`.
The commits in this branch do not need to be fully functional, but before creating a pull request, you should check that the final commit compiles and the automated tests (gtest) run successfully.
If the code passes the PR review, it can be merged to the 'develop' branch for testing.
(The idea is to test the changes in an accumulated way in the testnet, assuming that most changes work and do not require debugging. This should be easier than deploying each feature branch in the testnet separately.)
The branch should be deleted after merging with the PR.

4. `testnet/YYYY-MM-DD-FeatureOrReleaseName`: This is a temporary branch used for the testnet, building on feature branches or the `develop` branch. It contains at least one commit that adapts the code as required for running in the testnet.
We will delete testnet branches that have not been changed for one month or longer. And they may be cleaned up before by their owner (the developer who created it) to avoid cluttering the repository with unused branches.

5. `release/v1.XYZ`: This is a release branch for a specific version, usually for a new epoch.
It should be created from the 'develop' branch after testing the new features and agreeing on what is supposed to be included in the release.
This branch is then merged to the main branch via PR before creating the release on the main branch.
The branch may be deleted after merging.
However, it may be necessary to later recreate the branch if an old release needs to be fixed or updated, such as for creating the rollback version v1.190.1 for epoch 94.

For each release, there will be a tag like `v1.XYZ.N`.
The release description should contain the target epoch number and a short change log.

Mandatory update releases like adding SC or IPO should be published before Sunday (1/2 of the epoch), so that computors can catch up with AUX&MAIN mode and have the new version running the next epoch seamlessly (the following Wednesday).


## Coding guidelines

TODO


