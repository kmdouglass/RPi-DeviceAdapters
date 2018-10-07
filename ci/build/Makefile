# Builds and pushes the rpi-micromanager Docker build images.
#
# Based on:
# https://container-solutions.com/tagging-docker-images-the-right-way/
#
# Kyle M. Douglass, 2018
# kyle.m.douglass@gmail.com
#

NAME   := kmdouglass/rpi-micromanager
TAG    := $$(git log -1 --pretty=%H)
IMG    := ${NAME}:build-${TAG}
LATEST := ${NAME}:build

build:
	@docker build -t ${IMG} .
	@docker tag ${IMG} ${LATEST}

push:
	@docker push ${NAME}
