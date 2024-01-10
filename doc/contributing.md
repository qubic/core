# How to Contribute

## Contributing as an external developer

If you find bugs, typos, or other problems that can be fixed with a few changes, you are more than welcome to contribute these fixes with a pull request as follows.
For big changes, such as adding a smart contract, we recommended to discuss with one of the core developers first.

1. Create your own fork of this repository on GitHub.
2. Clone your fork on your local machine. Your remote repository on GitHub is called origin.
3. Add the original GitHub repository as a remote called upstream.
4. If you have created your fork a while ago, pull upstream changes into your local repository.
5. Create a new branch from `develop` or the current `release/epXX` branch.
6. Fix the bug, correct the typo, solve the problem, etc. Make sure to follow the coding guidelines below.
7. Commit your changes to your new branch. Make sure to use a concise but meaningful commit message.
8. Push your branch to the remote origin (your fork on Github).
9. From your fork on GitHub, open a pull request (PR) in your new branch targeting the `develop` or the current `release/epXX` branch of the original repo. In the PR, describe the problem and how your changes solve it.
10. If the PR reviewer requests further changes, push them to your branch. The PR will be updated automatically.
11. When your PR is approved and merged, you can pull the changes from upstream to your local repo and delete your extra branch.


## Development workflow / branches

We use a mixture of [GitFlow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) and a [trunk-based-process](https://www.atlassian.com/continuous-delivery/continuous-integration/trunk-based-development) with the following branches:

1. `main`: The code base which is officially deployed on computors. Releases are created from this branch.
2. `develop`: Our current testing branch, which will be automatically deployed to the test-net once active.
3. `release/epXX`: Our release branch for a specific epoch when we have specific tasks to work on (e.g. refactoring). May be deployed to test-net too or automatically by merging to dev.
4. `feature/DeveloperName_FeatureName`: When one of us is working on a specific feature that is unsure to be included into the next release, a feature branch should be created from `develop`. May be deployed to test-net.


## Coding guidelines

TODO


