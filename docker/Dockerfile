FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN mkdir /usr/src/ScarletDME && \
	apt-get update && \
	apt-get install -y gcc make libcrypt-dev && \
	rm -rf /var/lib/apt/lists/* && \
	groupadd qmusers && \
	useradd -G qmusers -d /usr/qmsys qmsys
COPY / /usr/src/ScarletDME
RUN chown -R qmsys:qmusers /usr/src/ScarletDME
RUN cp /usr/src/ScarletDME/scarlet.conf /etc/scarlet.conf
RUN chown qmsys:qmusers /etc/scarlet.conf
RUN usermod -a -G qmusers root
# Build unprivileged...
USER qmsys:qmusers
RUN (cd /usr/src/ScarletDME && (make clean || true))
RUN (cd /usr/src/ScarletDME && make)
# ...but install as root: `make install` creates $INSTROOT and chowns it, and
# neither is something the build user can do under /usr.
USER root:root
RUN (cd /usr/src/ScarletDME && make install)
WORKDIR /usr/qmsys
