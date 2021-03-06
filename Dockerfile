#ARG BASEOS
#FROM cornellcac/nix_${BASEOS}_base:2cd324f48c57f66d476a7894c6a957a78e49b116
FROM cornellcac/nix_alpine_base:2cd324f48c57f66d476a7894c6a957a78e49b116

USER root
ARG ADDUSER

COPY ContainerFiles/config.nix $HOME/.config/nixpkgs/
COPY ContainerFiles/dev-env.nix $ENVSDIR/
# Note, below is generated with:
# ssh-keyscan bitbucket.org > ContainerFiles/bitbucket_ssh_keyscan
COPY ContainerFiles/bitbucket_ssh_keyscan /tmp/
COPY Utils/persist-env.sh $ENVSDIR/
COPY id_rsa $HOME/.ssh/id_rsa
RUN chown -R $nixuser:$nixuser $ENVSDIR
RUN chown -R $nixuser:$nixuser $HOME/.ssh

#
# Initialize environment a bit for faster container spinup/use later
#
USER $nixuser
RUN $nixenv && cd /tmp && sh $ENVSDIR/persist-env.sh $ENVSDIR/dev-env.nix
RUN $nixenv && cd $HOME && \
  eval $(ssh-agent) && \
  ssh-add $HOME/.ssh/id_rsa && \
  cat /tmp/bitbucket_ssh_keyscan >> $HOME/.ssh/known_hosts && \
  git clone https://github.com/federatedcloud/WaterPaths.git && \
  cd WaterPaths && \
  git checkout aaf3c8ea9755dab072f310ab596cce3d387af8ce && \
  nix-shell -I ssh-config-file=./nix/.ssh-standard-config -I ssh-auth-sock=$SSH_AUTH_SOCK nix/shell.nix --run "make borg" && \
  rm -f $HOME/.ssh/id_rsa

# Testing
COPY nix/remora.nix $HOME/WaterPaths

USER root

#
# The below method isn't ideal, as it could break - better to copy in an sshd_config file
# or somehow use nix to configure sshd
# sed -i 's/.*PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
#
# alternatively ...:
# TODO: write a script to check if PermitRootLogin is already set, and replace it if so, else add it
#
ENV SSHD_PATH ""
RUN SSHD_PATH=$(su -c "$nixenv && nix-build '<nixpkgs>' --no-build-output --no-out-link -A openssh" "${nixuser:?}") && \
  mkdir -p /etc/ssh && cp "$SSHD_PATH/etc/ssh/sshd_config" /etc/ssh/sshd_config && \
  mkdir /var/run/sshd && \
  printf "PermitRootLogin yes\n" >> /etc/ssh/sshd_config && \
  id -u sshd || ${ADDUSER} sshd && \
  mkdir -p /var/empty/sshd/etc && \
  cd /var/empty/sshd/etc && \
  ln -s /etc/localtime localtime

# make mpi and orted executable symlinks on path
RUN for i in $(ls /nixenv/nixuser/.nix-profile/bin) ; do ln -s /nixenv/nixuser/.nix-profile/bin/"$i" /usr/bin ; done
RUN sed -i 's|^nixuser.*|nixuser:x:1000:1000::/home/nixuser:/nixenv/nixuser/.nix-profile/bin/bash|' /etc/passwd

USER $nixuser


# ------------------------------------------------------------
# Set-Up SSH with our Github deploy key
# ------------------------------------------------------------

ENV SSHDIR ${HOME}/.ssh/

RUN mkdir -p ${SSHDIR}

ADD ContainerFiles/ssh/config ${SSHDIR}/config
ADD ContainerFiles/ssh/id_rsa.mpi ${SSHDIR}/id_rsa
ADD ContainerFiles/ssh/id_rsa.mpi.pub ${SSHDIR}/id_rsa.pub
ADD ContainerFiles/ssh/id_rsa.mpi.pub ${SSHDIR}/authorized_keys

USER root

RUN passwd -d nixuser
RUN chmod -R 600 ${SSHDIR}* && \
    chown -R ${nixuser}:${nixuser} ${SSHDIR}

# ------------------------------------------------------------
# Configure ContainerFiles
# ------------------------------------------------------------

RUN rm -fr ${HOME}/.openmpi && mkdir -p ${HOME}/.openmpi
ADD ContainerFiles/default-mca-params.conf ${HOME}/.openmpi/mca-params.conf
RUN chown -R ${nixuser}:${nixuser} ${HOME}/.openmpi

ENV TRIGGER 1

#Copy this last to prevent rebuilds when changes occur in them:
COPY ContainerFiles/entrypoint* $ENVSDIR/

RUN chown $nixuser:$nixuser $ENVSDIR/entrypoint
ENV PATH="${PATH}:/usr/local/bin"

EXPOSE 22
ENTRYPOINT ["/bin/sh", "./entrypoint"]
