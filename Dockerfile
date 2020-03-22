FROM u16-opencv2

WORKDIR /home/developer/projects
ENV HOME /home/developer
ENV USER developer
USER developer
ENV PATH /home/developer/.local/bin:$PATH

# USER developer
CMD ["/bin/bash"]
