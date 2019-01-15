# 注意：构建时docker build的PATH参数须指定上此文件所在目录的上一级，
# 并用-f显式指定现Dockerfile文件，即docker build -f Dockerfile -t cpd:2 ..
# 如果在上一级执行，则应该用docker build -f docker/Dockerfile -t cpd:2 .


FROM centos:7

# MAINTAINER
MAINTAINER valueho hjl_mvp@126.com

ADD cpp_sdk /cppcloud/cpp_sdk/
ADD common /cppcloud/common/
ADD server /cppcloud/server/
ADD docker /cppcloud/docker


ENV LD_LIBRARY_PATH /cppcloud/common:/cppcloud/cpp_sdk:$LD_LIBRARY_PATH
ENV LIBRARY_PATH /cppcloud/common:/cppcloud/cpp_sdk:$LIBRARY_PATH
ENV PATH $PATH:/cppcloud/docker
ENV LOG2STD 1

# running required command
RUN ln -sf /usr/share/zoneinfo/Hongkong /etc/localtime \
        && yum install -y gcc gcc-c++ glibc make 
## autoconf openssl openssl-devel 

WORKDIR /cppcloud/docker
RUN make all \
        && chmod +x sample_call_model.sh \
        && ln -s sample_call_model.sh sample_conf \
        && ln -s sample_call_model.sh sample_prvd \
        && ln -s sample_call_model.sh sample_tcp_invk \
        && ln -s sample_call_model.sh sample_http_invk \
        && ln -s sample_call_model.sh agent_prvd \
        && ln -s /cppcloud/server/cppcloud_serv cppcloud_serv \
        && chmod +x docker-entrypoint.sh && /usr/bin/cp -r /cppcloud/server/conf .


CMD ["help"]
ENTRYPOINT ["./docker-entrypoint.sh"]


EXPOSE 4800
VOLUME ["/cppcloud/docker/conf", "/cppcloud/docker/log"]
