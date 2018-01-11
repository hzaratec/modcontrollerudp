FROM arm32v7/gcc:7
RUN apt-get update && apt-get install -y cgroup-bin \
    cgroup-tools\
    cgroupfs-mount\
    libstdc++6\
    sudo\
    cron\ 
    libcgroup*
ADD crontab /etc/cron.d/app-cron
RUN chmod 0644 /etc/cron.d/app-cron
RUN touch /var/log/cron.log
COPY /modcontroller/ /home/pi/pfm
WORKDIR /home/pi/pfm
RUN make
EXPOSE 50000
CMD ["bash","-c","/home/pi/pfm/controller/controller","&"] 
