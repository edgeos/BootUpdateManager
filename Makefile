.PHONY: all build version all-build all-image all-push build-shell build-env image push test clean image-clean build-env-clean bin-clean
# Build tools
#
# Targets (see each target for more information):
#   build:	builds binaries for specified architecture
#   image:	builds the docker image
#   test:	runs linting, unit tests etc.
#   scan:	runs static analysis tools
#   clean:	removes build artifacts and images
#   push:	pushes image to registry
#
#   all-build:	builds binaries for all target architectures
#   all-images:	builds the docker images for all target architectures
#   all-push:	pushes the images for all architectures to registry
#


###
### Customize  these variables
###

SHELL = /bin/bash -x

NAME := BUM

# Where to push the docker image.
REGISTRY ?= registry.gear.ge.com/predix_edge

# Which architecture to build - see $(ALL_ARCH) for options.
ARCH ?= amd64

# This version-strategy uses git tags to set the version string
#VERSION := $(shell git describe --tags --always --dirty)
VERSION := 1.0.0

# A name to give your build image. 
# Dockerfile.build defines how to build the image,
# and the `build-env` rule builds it.
BUILD_IMAGE ?= bum-builder-c-$(ARCH)

# Name of Coverity Conneect stream under Predix Edge project.
# Contact edge.appdevdevops@ge.com to create a new stream for your project.
COVERITY_STREAM := c-template

# Type of gcc compilation. Interpritted by build/build.sh. Affects `build` and `image` rules.
# Select from executable, loader, all
BUILD_TYPE=all

###
### These variables should not need tweaking.
###

# Platform specific USER  and proxy crud:
# On linux, run the container with the current uid, so files produced from
# within the container are owned by the current user, rather than root.
#
# On OSX, don't do anything with the container user, and let boot2docker manage
# permissions on the /Users mount that it sets up
DOCKER_USER := $(shell if [[ "$$OSTYPE" != "darwin"* ]]; then USER_ARG="--user=`id -u`"; fi; echo "$$USER_ARG")
PROXY_ARGS := -e http_proxy=$(http_proxy) -e https_proxy=$(https_proxy) -e no_proxy=$(no_proxy)
BUILD_ARGS := --build-arg http_proxy=$(http_proxy) --build-arg https_proxy=$(https_proxy) --build-arg no_proxy=$(no_proxy)

# directories which hold app source (not vendored)
SRC_DIRS := src

TEST_DIRS := test # directories which hold test source

ALL_ARCH := amd64 i386 arm aarch64

BASEIMAGE?=ubuntu:16.04

IMAGE := $(REGISTRY)/$(NAME)-$(ARCH)

# Find all files under the source directory, if any of them changes, execute the lower Makefile
PROJECT_SOURCE = $(foreach DIR, $(SRC_DIRS), $(shell find $(DIR) -type f))

# Default target
all: build

# Print listed version
version:
	@echo $(VERSION)

###
### Architecture-specified rules
###

# Build for a specific architecture in a Docker container and copy to volume mount
# Example: `build-amd64` is equivalent to `build`
build-%:
	@$(MAKE) --no-print-directory ARCH=$* build

# Builds the docker image for the specified architecture and tags it appropriately
# Example: `image-amd64` is equivalent to `image`
image-%:
	@$(MAKE) --no-print-directory ARCH=$* image

# Pushes the built docker image to the specified registry for specified architecture
# Example: `push-amd64` is equivalent to `push`
push-%:
	@$(MAKE) --no-print-directory ARCH=$* push

# Cleans the docker images and containers, as well as any artifacts built.
# Example: `clean-amd64` is equivalent to `clean`
clean-%:
	@$(MAKE) --no-print-directory ARCH=$* clean

# Cleans just runtime docker images and containers.
# Example: `image-clean-amd64` is equivalent to `image-clean`
image-clean-%:
	@$(MAKE) --no-print-directory ARCH=$* image-clean

# Cleans just build artifacts
# Example: `bin-clean-amd64` is equivalent to `bin-clean`
bin-clean-%:
	@$(MAKE) --no-print-directory ARCH=$* bin-clean

# Builds all the binaries in a Docker container and copies to volume mount
all-build: $(addprefix build-, $(ALL_ARCH))

# Builds all docker images and tags them appropriately
all-image: $(addprefix image-, $(ALL_ARCH))

# Builds and pushes all images to registry
all-push: $(addprefix push-, $(ALL_ARCH))

# Cleans images, containers, and atrifacts for all Architectures
all-clean: $(addprefix clean-, $(ALL_ARCH))

# Cleans just the runtime images for all Architectures
all-image-clean: $(addprefix image-clean-, $(ALL_ARCH))

# Cleans just the artifacts for all Architectures
all-bin-clean: $(addprefix bin-clean-, $(ALL_ARCH))


###
### Default architecture rules. Default set by ARCH value
###

# Default build target
build: bin/$(ARCH)/$(NAME)

# Builds source and outputs to bin directory via volume mount
bin/$(ARCH)/$(NAME): bin/$(ARCH) .image-$(BUILD_IMAGE) $(PROJECT_SOURCE)
	@echo "building:    $@"
	@echo "buildtype:   $BUILD_TYPE"
	@echo $(DOCKER_USER)
	@docker run                                 \
		-t                                      \
		--rm                                    \
		$(DOCKER_USER)                          \
		$(PROXY_ARGS)                           \
		-v $$(pwd):/home/edge/$(NAME)           \
		-w /home/edge/$(NAME)                   \
		-e ARCH=$(ARCH)                         \
		-e VERSION=$(VERSION)                   \
		-e NAME=$(NAME)                         \
		-e SRC_DIRS="$(SRC_DIRS)"               \
		-e BUILD_TYPE=$(BUILD_TYPE)             \
		$(BUILD_IMAGE)                          \
		/bin/bash -c "                          \
		./build/build.sh                        \
		"
	@echo "built"

# Interactive session in build container
build-shell: bin/$(ARCH) .image-$(BUILD_IMAGE)
	@echo "Entering build shell..."
	@echo $(DOCKER_USER)
	@docker run                                 \
		-it                                     \
		$(DOCKER_USER)                          \
		$(PROXY_ARGS)                           \
		-v $$(pwd):/home/edge/$(NAME)           \
		-w /home/edge/$(NAME)                   \
		$(BUILD_IMAGE)                          \
		/bin/bash

# Creates contianer to build code
build-env: .image-$(BUILD_IMAGE)
.image-$(BUILD_IMAGE): Dockerfile.build
	@echo "building Docker image with build environment"
	@sed -e "s|ARG_FROM|$(BASEIMAGE)|g" Dockerfile.build > .dockerfile-build-$(ARCH)
	@docker build \
		$(BUILD_ARGS) \
		-t $(BUILD_IMAGE) \
		-f .dockerfile-build-$(ARCH) .
	@docker images -q $(BUILD_IMAGE) > $@

# Create output directory for compiled source
bin/$(ARCH):
	@mkdir -p $@


# Packages compiled binary into a Docker container
DOTFILE_IMAGE = $(subst /,_,$(IMAGE))-$(VERSION)
image: .image-$(DOTFILE_IMAGE) image-name
.image-$(DOTFILE_IMAGE): bin/$(ARCH)/$(NAME) Dockerfile.in
	@./build/dockerfile_autogen.sh \
		--build-type $(BUILD_TYPE) \
		--arch $(ARCH) \
#		--name $(NAME) \
		--baseimage $(BASEIMAGE) \
		--version $(VERSION)
	@docker build -t $(IMAGE):$(VERSION) -f .dockerfile-$(ARCH) .
	@docker images -q $(IMAGE):$(VERSION) > $@
image-name:
	@echo "image: $(IMAGE):$(VERSION)"

# Push image to Docker Trsted Registry
# Requires authorization.
push: .push-$(DOTFILE_IMAGE) push-name
.push-$(DOTFILE_IMAGE): .image-$(DOTFILE_IMAGE)
	@gcloud docker push $(IMAGE):$(VERSION)
	@docker images -q $(IMAGE):$(VERSION) > $@
push-name:
	@echo "pushed: $(IMAGE):$(VERSION)"

# Execute run-time tests, such as unnit tests
test: bin/$(ARCH) .image-$(BUILD_IMAGE)
	@docker run                                 \
	    -t                                      \
	    $(DOCKER_USER)                          \
	    $(PROXY_ARGS)                           \
	    -v $$(pwd):/home/edge/$(NAME)           \
	    -w /home/edge/$(NAME)                   \
	    $(BUILD_IMAGE)                          \
	    /bin/bash -c "                          \
	        ./test/test.sh $(TEST_DIRS)         \
	    "
# Execute static analysis. 
# Our Jenkins CI/CD server has a license for Coverity. 
scan: .image-$(BUILD_IMAGE)
ifeq ($(THISISJENKINS),YES) # If running on jenkins, then scan with Coverity too
	@echo "Runnning Coverity"
	@docker run                                 \
		-t                                      \
		--rm                                    \
		-v $$HOME/cov:/cov:ro                   \
		-v $$(pwd):$$(pwd)                      \
		-w $$(pwd)                              \
		-e ARCH=$(ARCH)                         \
		-e VERSION=$(VERSION)                   \
		-e NAME=$(NAME)                         \
		-e SRC_DIRS=$(SRC_DIRS)                 \
		-e COVERITY_HOST=$(COVERITY_HOST)       \
		-e COVERITY_PORT=$(COVERITY_PORT)       \
		-e COVERITY_STREAM=$(COVERITY_STREAM)   \
		-e COVERITY_USER=$(COVERITY_USER)       \
		-e COVERITY_PASSWORD=$(COVERITY_PASSWORD) \
		$(BUILD_IMAGE)                          \
		/bin/bash -c "                          \
			./test/scan-coverity.sh             \
			"
else
	@echo "Running scans"
	@echo "   TODO - scans"
	@echo "Done running scans"
endif

###
### Clean up rules
###

clean: image-clean bin-clean build-env-clean

image-clean:
	@if [ $(shell docker ps -a | grep $(IMAGE) | wc -l) != 0 ]; then \
		docker ps -a | grep $(IMAGE) | awk '{print $$1 }' | xargs docker rm -f; \
	fi
	@if [ $(shell docker images | grep $(IMAGE) | wc -l) != 0 ]; then \
		docker images | grep $(IMAGE) | awk '{print $$3}' | xargs docker rmi -f || true; \
	fi
	rm -rf .image-$(DOTFILE_IMAGE) .dockerfile-$(ARCH) .push-*

build-env-clean:
	@if [ $(shell docker ps -a | grep $(BUILD_IMAGE) | wc -l) != 0 ]; then \
		docker ps -a | grep $(BUILD_IMAGE) | awk '{print $$1 }' | xargs docker rm -f; \
	fi
	@if [ $(shell docker images | grep $(BUILD_IMAGE) | wc -l) != 0 ]; then \
		docker images | grep $(BUILD_IMAGE) | awk '{print $$3 }' | xargs docker rmi -f || true; \
	fi
	rm -rf .image-$(BUILD_IMAGE) .dockerfile-build-$(ARCH)

bin-clean:
	rm -rf "bin/$(ARCH)" "bin/edk2"

