FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y openssh-server rsync build-essential g++ cmake git gdb python3 pipx

RUN  echo 'ubuntu:password' | chpasswd && \
     echo 'root:password' | chpasswd

RUN mkdir /var/run/sshd && \
    sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd && \
    echo "export VISIBLE=now" >> /etc/profile

RUN runuser -l ubuntu -c 'pipx install conan' && \
    runuser -l ubuntu -c 'pipx ensurepath' && \
    runuser -l ubuntu -c 'conan profile detect'

RUN runuser -l ubuntu -c 'cd'' && \
    runuser -l ubuntu -c 'git clone https://github.com/maxoodf/colet_test.git' && \
    runuser -l ubuntu -c 'cd colet_test' && \
    runuser -l ubuntu -c 'cmake -B cmake-build-debug-colet -S . -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./cmake/conan_provider.cmake -DCMAKE_BUILD_TYPE=Debug' && \
    runuser -l ubuntu -c 'cmake -B cmake-build-release-colet -S . -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./cmake/conan_provider.cmake -DCMAKE_BUILD_TYPE=Release' && \
    runuser -l ubuntu -c 'cmake --build cmake-build-debug-colet --config Debug' && \
    runuser -l ubuntu -c 'cmake --build cmake-build-release-colet --config Release' && \


EXPOSE 22

CMD ["/usr/sbin/sshd", "-D"]
