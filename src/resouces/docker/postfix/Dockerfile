FROM yeongroyun/software_security_chat_service_ubuntu1604_32bit_postfix

EXPOSE 25
COPY run.sh /run.sh
RUN chmod +x /run.sh

ENTRYPOINT [ "/run.sh" ]
CMD ["/bin/bash"]
