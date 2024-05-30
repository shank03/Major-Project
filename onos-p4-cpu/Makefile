mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
curr_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include util/docker/Makefile.vars

onos_url := http://localhost:8181/onos
onos_curl := curl --fail -sSL --user onos:rocks --noproxy localhost
app_name := org.onosproject.major-shank

default:
	$(error Please specify a make target (see README.md))

_docker_pull_all:
	docker pull ${BMV2_BUILD_IMG}@${BMV2_BUILD_SHA}
	docker tag ${BMV2_BUILD_IMG}@${BMV2_BUILD_SHA} ${BMV2_BUILD_IMG}
	docker pull ${ONOS_IMG}@${ONOS_SHA}
	docker tag ${ONOS_IMG}@${ONOS_SHA} ${ONOS_IMG}
	docker pull ${P4C_IMG}@${P4C_SHA}
	docker tag ${P4C_IMG}@${P4C_SHA} ${P4C_IMG}
	docker pull ${STRATUM_BMV2_IMG}@${STRATUM_BMV2_SHA}
	docker tag ${STRATUM_BMV2_IMG}@${STRATUM_BMV2_SHA} ${STRATUM_BMV2_IMG}
	docker pull ${SSHPASS_IMG}@${SSHPASS_SHA}
	docker tag ${SSHPASS_IMG}@${SSHPASS_SHA} ${SSHPASS_IMG}

deps: _docker_pull_all

_start:
	$(info *** Starting ONOS and Mininet (${TOPO_PY})... )
	@mkdir -p tmp/onos
	@TOPO_PY=${TOPO_PY} docker-compose up -d

start: TOPO_PY := topo-v4.py
start: _start

stop:
	$(info *** Stopping ONOS and Mininet...)
	@TOPO_PY=foo docker-compose down -t0

restart: reset start

onos-cli:
	$(info *** Connecting to the ONOS CLI... password: rocks)
	$(info *** Top exit press Ctrl-D)
	@ssh -o "UserKnownHostsFile=/dev/null" -o "StrictHostKeyChecking=no" -o LogLevel=ERROR -p 8101 onos@localhost

onos-log:
	docker-compose logs -f onos

onos-ui:
	open ${onos_url}/ui

mn-cli:
	$(info *** Attaching to Mininet CLI...)
	$(info *** To detach press Ctrl-D (Mininet will keep running))
	-@docker attach --detach-keys "ctrl-d" $(shell docker-compose ps -q mininet) || echo "*** Detached from Mininet CLI"

mn-log:
	docker logs -f mininet

_netcfg:
	$(info *** Pushing ${NETCFG_JSON} to ONOS...)
	${onos_curl} -X POST -H 'Content-Type:application/json' \
		${onos_url}/v1/network/configuration -d@./mininet/${NETCFG_JSON}
	@echo

netcfg: NETCFG_JSON := netcfg.json
netcfg: _netcfg

reset: stop
	-rm -rf ./tmp

clean:
	-rm -rf p4src/build
	-rm -rf app/target
	-rm -rf app/src/main/resources/bmv2.json
	-rm -rf app/src/main/resources/p4info.txt

p4-build: p4src/main.p4
	$(info *** Building P4 program...)
	@mkdir -p p4src/build
	docker run --rm -v ${curr_dir}:/workdir -w /workdir ${P4C_IMG} \
		p4c-bm2-ss --arch v1model -o p4src/build/bmv2.json \
		--p4runtime-files p4src/build/p4info.txt --Wdisable=unsupported --emit-externs \
		p4src/main.p4
	@echo "*** P4 program compiled successfully! Output files are in p4src/build"

bmv2-extern:
	$(info *** Compiling extern function...)
	@rm -rf extern/cpu.so
	docker run --rm -v ${curr_dir}:/workdir -w /workdir ${BMV2_BUILD_IMG} \
		g++ -Wall -Wextra -g -O2 -fPIC -shared extern/cpu.cpp -o extern/cpu.so

_copy_p4c_out:
	$(info *** Copying p4c outputs to app resources...)
	@mkdir -p app/src/main/resources
	cp -f p4src/build/p4info.txt app/src/main/resources/
	cp -f p4src/build/bmv2.json app/src/main/resources/

_mvn_package:
	$(info *** Building ONOS app...)
	@mkdir -p app/target
	@cd app && mvn clean package

app-build: bmv2-extern p4-build _copy_p4c_out _mvn_package
	$(info *** ONOS app .oar package created succesfully)
	@ls -1 app/target/*.oar

app-install:
	$(info *** Installing and activating app in ONOS...)
	${onos_curl} -X POST -HContent-Type:application/octet-stream \
		'${onos_url}/v1/applications?activate=true' \
		--data-binary @app/target/major-shank-1.0-SNAPSHOT.oar
	@echo

app-uninstall:
	$(info *** Uninstalling app from ONOS (if present)...)
	-${onos_curl} -X DELETE ${onos_url}/v1/applications/${app_name}
	@echo

app-reload: app-uninstall app-install

CSVS := $(wildcard **/*.csv)
TARGETS := $(CSVS:.csv=.png)

py-venv:
	$(info *** Installing python venv deps $< ...)
	python3 -m venv .venv
	@.venv/bin/python -m pip install matplotlib pandas

plt-graph: py-venv $(TARGETS)

%.png: %.csv
	$(info *** Generating graphs for $< ...)
	@.venv/bin/python records/graph.py $<

run:
	make stop
	make clean
	make app-build
	make start
	sleep 90
	make app-reload
	sleep 10
	# util/onos-cmd app activate proxyarp
	# util/onos-cmd app activate hostprovider
	# util/onos-cmd app activate lldpprovider
	make netcfg
	sleep 5
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.3
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.4
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.5
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.6
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.7
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.8
	-util/mn-cmd h10 ping -c1 -i.1 192.168.1.9
	@echo Network environment in execution
