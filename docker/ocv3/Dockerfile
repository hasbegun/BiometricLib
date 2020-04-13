FROM u18-ocv3-qt5

WORKDIR /home/developer/projects
RUN mkdir /home/developer/.ssh && chown developer:developer /home/developer/.ssh

# add basic files
ADD res/gitconfig /home/developer/.gitconfig
ADD res/commit_template.txt /home/developer/.commit_template.txt
ADD res/bashrc /home/developer/.bashrc
ADD res/docker_dev /home/developer/.ssh/docker_dev
ADD res/docker_dev.pub /home/developer/.ssh/docker_dev.pub
RUN chown developer:developer \
    /home/developer/.gitconfig \
    /home/developer/.commit_template.txt \
    /home/developer/.bashrc \
    /home/developer/.ssh/docker_dev \
    /home/developer/.ssh/docker_dev.pub

# Use C.UTF-8 locale to avoid issues with ASCII encoding
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

ENV HOME /home/developer
ENV USER developer
USER developer
ENV PATH /home/developer/.local/bin:$PATH
# Avoid first use of sudo warning. c.f. https://askubuntu.com/a/22614/781671
RUN touch $HOME/.sudo_as_admin_successful

# Add key to ssh known hosts.
# Keep this line as a line. If split, the key will be lost and will not work.
RUN eval "$(ssh-agent)" && \
    ssh-keyscan github.com >> /home/developer/.ssh/known_hosts && \
    ssh-add -k /home/developer/.ssh/docker_dev && \
    git clone git@github.com:hasbegun/BiometricLib.git && \
	cd BiometricLib && ./build.sh && \
    rm -rf /home/developer/projects/BiometricLib

CMD ["/bin/bash"]
