FROM u18-ocv3-qt5

WORKDIR /home/developer/projects

# add basic files
ADD res/gitconfig /home/developer/.gitconfig
ADD res/commit_template.txt /home/developer/.commit_template.txt
ADD res/bashrc /home/developer/.bashrc
RUN chown developer:developer \
    /home/developer/.gitconfig \
    /home/developer/.commit_template.txt \
    /home/developer/.bashrc

# Use C.UTF-8 locale to avoid issues with ASCII encoding
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

ENV HOME /home/developer
ENV USER developer
USER developer
ENV PATH /home/developer/.local/bin:$PATH
# Avoid first use of sudo warning. c.f. https://askubuntu.com/a/22614/781671
RUN touch $HOME/.sudo_as_admin_successful

CMD ["/bin/bash"]
