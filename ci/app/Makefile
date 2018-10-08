# Builds and pushes the rpi-micromanager Docker app images.
#
# Based on:
# https://container-solutions.com/tagging-docker-images-the-right-way/
#
# Kyle M. Douglass, 2018
# kyle.m.douglass@gmail.com
#

NAME   := kmdouglass/rpi-micromanager
TAG    := $$(git log -1 --pretty=%H)
IMG    := ${NAME}:${TAG}
LATEST := ${NAME}:latest

build:
	cp -r /opt/rpi-micromanager/build $(shell pwd)
	@docker build -t ${IMG} .
	@docker tag ${IMG} ${LATEST}
	rm -rf build

push:
	@docker push ${NAME}
