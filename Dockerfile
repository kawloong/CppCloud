FROM centos:7

# MAINTAINER
MAINTAINER valueho hjl_mvp@126.com

ADD common /cppcloud/common/
ADD client_c /cppcloud/client_c/
ADD server /cppcloud/server/
ADD Makefile /cppcloud/


ENV LD_LIBRARY_PATH /cppcloud/common:$LD_LIBRARY_PATH
ENV LIBRARY_PATH /cppcloud/common:$LIBRARY_PATH

# running required command
RUN ln -sf /usr/share/zoneinfo/Hongkong /etc/localtime \
        && yum install -y gcc gcc-c++ glibc make 
## autoconf openssl openssl-devel 

WORKDIR /cppcloud
RUN ["make", "all"]


CMD ["1"]
WORKDIR /cppcloud/server
ENTRYPOINT ["./cppcloud_serv", "-i"]


EXPOSE 4800
VOLUME ["/cppcloud/server/conf"]
