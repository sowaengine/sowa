#!/bin/sh

set -x

protoc -I=src/proto --cpp_out=src/proto src/proto/project_settings.proto src/proto/scene.proto