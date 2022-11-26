FROM yeongroyun/software_security_chat_service_ubuntu1604_32bit_mysql8

EXPOSE 3306
EXPOSE 33060
COPY run.sh /run.sh
RUN chmod +x /run.sh
ENTRYPOINT [ "/run.sh" ]
