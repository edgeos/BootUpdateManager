# C application template

This is a template project for a C application, which demonstrates some of the reccommended techniques for developing c-based Predix Edge services.
This template is provided as launch point for basic projects, and as such makes assumptions and simplifications. With this in mind, You are encouraged to clone this repo and use, modify, or discard to fit your needs.

This has been tested on Linux.  Depends on Docker to build.

To copy this repository run the following commands in your terminal:

~~~bash
$ git clone -b develop --single-branch --depth 1 git@github.build.ge.com:PredixEdgeProjects/template-c.git
~~~

This will clone only the latest iteration of master branch, and remove the connection to this repository so that you can start fresh with your own repository.

Then set the remote to your GitHub repository, add your values for the ```<...>```-tagged sections:

~~~bash 
git remote set-url origin git@github.build.ge.com:<GitHub Orgainzation>/<Repository Name>.git
~~~

## Basic Configuration & Philosophy of Use

Follow the directions in the following files to configure your project:

1. Makefile  
    * Runs on host, and is main interface to building the project  
    * Used for general configuration  
    * Build rules on the highest level, relating to Docker and containerization  
2. build/build.sh  
    * Runs in the container defined by `make build-env` rule and Dockerfile.build  
    * Used for executing mechanics of the type of compilation you want (see `BUILD_TYPE` variable)  
3. build/Makefile.gcc  
    * Runs in the container defined by `make build-env` rule and Dockerfile.build  
    * Flexible gcc interface for building the project regardless of build type
4. build/dockerfile_autogen.sh  
    * Script run by the  `make image` rule  
    * Populates Dockerfile.in to define packaging the compiled output in a container  
5. Dockerfile.in  
    * A dockerfile that is used to build image that packages compiled output  
    * auto-populated by build/dockerfile_autogen.sh  
6. Dockerfile.build  
    * A dockerfile describing how to build the build environment image
7. src directory  
    * Home of source and header files  
    * Build process recursively scans this directory for .h and .c files  
8. test directory  
    * Scripts that define how to perform static analysis and testing on the plaintext code, compiled artifacts, and containers  
    * Use Makefile file's `make test` rule to define how these tests are started  
9. bin directory  
    * Auto-generated directory for compiled output  

## Building

## `binfmt_misc` register

In order to build with multiarch, you must register `qemu-*-static` for all supported processors except the current one

* `docker run --rm --privileged multiarch/qemu-user-static:register`

To remove, same as above, but remove all registered `binfmt_misc` before

* `docker run --rm --privileged multiarch/qemu-user-static:register --reset`

Run `make` or `make build` to compile your app.  This will use a Docker image
to build your app, with the current directory volume-mounted into place.  This
will store incremental state for the fastest possible build.  Run `make
all-build` to build for all architectures.

Run `make image` to build the image  It will calculate the image
tag based on the most recent git tag, and whether the repo is "dirty" since
that tag (see `make version`).  Run `make all-image` to build images
for all architectures.

Run `make test` to test the image.  Format checks, lint, and unit tests will be
executed.

Run `make push` to push the container image to `REGISTRY`.  Run `make all-push`
to push the container images for all architectures.

Run `make clean` to clean up.

## Jenkins Integration

This section will cover how to set up a GitHub project for testing on [http://ci.edge.research.ge.com](http://ci.edge.research.ge.com).

### The EdgeJenkinsCI user permissions

The Jekins CI should be used to build projects in the PredixEdge, PredixEdgeOS, PredixEdgeProjects, and PredixEdgePlayground organizations.  
You should make sure that the EdgeJenkinsCI user (SSO: 502712920) is an **admin** on your repo.  

Make sure the EdgeJenkinsCI (502712920) user is an **admin** contributor on your repo:  
    1. Navigate to your GitHub repo
    2. Add ```EdgeJenkinsCI``` or ```502712920``` as an ```Admin``` in Settings > Collaborators & Teams > Collaborators```

### The Jenkinsfile

In order for the Jenkins CI to build your repo you must have a Jenkinsfile on each branch in the base of your repository. This file is used to instruct Jenkins how to perform its test.
This will usually follow the format provided in this repo, and is predicated on a properly made Makefile in the base of the repo.

For more information about Jenkinsfile syntax, see [https://jenkins.io/doc/book/pipeline/jenkinsfile/](https://jenkins.io/doc/book/pipeline/jenkinsfile/)

### Jenkins Setup

Sign up for a Jenkins account by clicking on ```sign up``` in the upper right of [http://ci.edge.research.ge.com](http://ci.edge.research.ge.com).
With this user account you should be able to view the results of your builds (and everyone elses). If you have issues or should you want more priviledges please email <a href="mailto:edge.appdevdevops@ge.com">edge.appdevdevops@ge.com</a> with your need.

The rest should be automatic. If your repo in the 4 supported organiztions has any branches with Jenkinsfile, then the CI Jenkins server will build those branches whenever:

1. A change is made on the branch
2. A pull request is made from a branch with a Jenkinsfile (all branches should have Jenkinsfiles)

### GitHub Restriction Setup

You can set up rules in GitHub to dictate who can push changes to which branch, and also to block merges that have not passed Jenkins CI tests. It is reccomended that you do this at least for the master and develop branch.

Create protected branches, once for develop and once for master:


0. Create a simple pull request on your repository in order to sync GitHub "status checks" with Jenkins. You do not have to merge the pull-request, just generate it.
1. ```<your repository>``` > Settings > Branches > Protected branches > Choose a branch > develop (repeat for  branch: master)
2. Check the "Protect this branch" > Require status checks to pass before merging > Require branches to be up to date before merging > ```continuous-integration/jenkins/pr-merge```
3. Check the "Protect this branch" > Restrict who can push to this branch
4. Save changes

What pull request triggered job looks like when done right from the GitHub pull request page:

Pre-pass:

![pre-test](/img/pull-request-pre-check.png)

Post-pass:

![post-test](/img/pull-request-post-check.png)

## A Failing Test

Failing-test:

![failing-test](/img/pull-request-fail.png)

Note: If you have a failing test, and you are the administrator on the repo, then you can still force a merge, **but you should not**!

If you have a failing test, then you should check why it broke. There are two ways to check the cause of a break:

1. Test locally (Reccommended first step)
    1. Build and and test lo cally in your development environment.  The errors should generally occur locally if they are encountered in Jenkins.  **Note**: code should regularly be tested locally quickly identify and resolve errors.
2. See Jenkins Output
    1. Click on ```Details``` as circled in red in the image, which will open the Jenkins Build, and click on ```Console Output```.
    2. Log in to Jenkins, click on the failing Job, and click on ```Console Output```.  

## Corrections and errors

Should you find any inconsistencies or errors in this document, kindly do one of the following:

1. Fork the repo, create your fixes, create a pull request with an explanation.
2. Create an issue on the repo from the ```Issues``` tab above the repo file navigator
3. Email <a href="mailto:edge.appdevdevops@ge.com">edge.appdevdevops@ge.com</a>
