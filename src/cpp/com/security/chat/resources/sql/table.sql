DROP TABLE IF EXISTS room_participant;
DROP TABLE IF EXISTS invitation;
DROP TABLE IF EXISTS chat_room;
DROP TABLE IF EXISTS chat_password;
DROP TABLE IF EXISTS chat_user;
DROP TABLE IF EXISTS company;

CREATE TABLE `company` (
	`company_id`	INT	NOT NULL ,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`name`	VARCHAR(100) UNIQUE	NOT NULL
);

CREATE TABLE `chat_user` (
	`user_id`	INT	NOT NULL ,
	`company_id`	INT	NOT NULL,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`name`	VARCHAR(100)	NOT NULL,
	`role`	TEXT	NOT NULL,
	`email` VARCHAR(500) UNIQUE NOT NULL
);

CREATE TABLE `chat_room` (
	`room_id`	INT	NOT NULL ,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`deleted_at`	DATETIME	NULL	COMMENT '백업 기간을 제공하기 위해 채팅방 삭제 요청이 들어온 시간을 저장한다.',
	`name`	VARCHAR(100) UNIQUE	NOT NULL
);

CREATE TABLE `room_participant` (
	`participant_id`	INT	NOT NULL ,
	`room_id`	INT	NOT NULL,
	`user_id`	INT	NOT NULL,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`role`	VARCHAR(100)	NOT NULL	COMMENT 'Host와 Guest 구분'
);

CREATE TABLE `chat_password` (
	`pw_id`	INT	NOT NULL ,
	`company_id` INT NULL,
	`user_id`	INT NULL,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`salt`	TEXT	NOT NULL,
	`hashed_pw`	TEXT	NOT NULL
);

CREATE TABLE `invitation` (
	`invitation_id`	INT	NOT NULL ,
	`room_id`	INT	NOT NULL,
	`user_id`	INT	NOT NULL,
	`created_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`last_modified_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`expired_at`	DATETIME	NOT NULL	DEFAULT NOW(),
	`password`	TEXT	NOT NULL	COMMENT '난수로 생성된 일회용 비밀번호'
);

ALTER TABLE `company` ADD CONSTRAINT `PK_COMPANY` PRIMARY KEY (
	`company_id`
);

ALTER TABLE `chat_user` ADD CONSTRAINT `PK_CHAT_USER` PRIMARY KEY (
	`user_id`,
	`company_id`
);

ALTER TABLE `chat_room` ADD CONSTRAINT `PK_CHAT_ROOM` PRIMARY KEY (
	`room_id`
);

ALTER TABLE `room_participant` ADD CONSTRAINT `PK_ROOM_PARTICIPANT` PRIMARY KEY (
	`participant_id`,
	`room_id`,
	`user_id`
);

ALTER TABLE `chat_password` ADD CONSTRAINT `PK_USER_PASSWORD` PRIMARY KEY (
	`pw_id`
);

ALTER TABLE `invitation` ADD CONSTRAINT `PK_INVITATION` PRIMARY KEY (
	`invitation_id`,
	`room_id`,
	`user_id`
);

ALTER TABLE company MODIFY company_id INT NOT NULL AUTO_INCREMENT;
ALTER TABLE chat_user MODIFY user_id INT NOT NULL AUTO_INCREMENT;
ALTER TABLE chat_room MODIFY room_id INT NOT NULL AUTO_INCREMENT;
ALTER TABLE room_participant MODIFY participant_id INT NOT NULL AUTO_INCREMENT;
ALTER TABLE chat_password MODIFY pw_id INT NOT NULL AUTO_INCREMENT;
ALTER TABLE invitation MODIFY invitation_id INT NOT NULL AUTO_INCREMENT;

ALTER TABLE `chat_user` ADD CONSTRAINT `FK_company_TO_chat_user_1` FOREIGN KEY (
	`company_id`
)
REFERENCES `company` (
	`company_id`
);

ALTER TABLE `room_participant` ADD CONSTRAINT `FK_chat_room_TO_room_participant_1` FOREIGN KEY (
	`room_id`
)
REFERENCES `chat_room` (
	`room_id`
);

ALTER TABLE `room_participant` ADD CONSTRAINT `FK_chat_user_TO_room_participant_1` FOREIGN KEY (
	`user_id`
)
REFERENCES `chat_user` (
	`user_id`
);

ALTER TABLE `chat_password` ADD CONSTRAINT `FK_chat_user_TO_chat_password_1` FOREIGN KEY (
	`user_id`
)
REFERENCES `chat_user` (
	`user_id`
);

ALTER TABLE `chat_password` ADD CONSTRAINT `FK_company_TO_chat_password_1` FOREIGN KEY (
	`company_id`
)
REFERENCES `company` (
	`company_id`
);

ALTER TABLE `invitation` ADD CONSTRAINT `FK_chat_room_TO_invitation_1` FOREIGN KEY (
	`room_id`
)
REFERENCES `chat_room` (
	`room_id`
);

ALTER TABLE `invitation` ADD CONSTRAINT `FK_chat_user_TO_invitation_1` FOREIGN KEY (
	`user_id`
)
REFERENCES `chat_user` (
	`user_id`
);

INSERT INTO company(name) VALUES ('company');
INSERT INTO chat_password(company_id, salt, hashed_pw) VALUES(1, 'd7C4D5VNDBMyeNjQtLWKU8kTadIc16cV8P3s2iUSceJWGsb286hULftdS7NpW7vunpAhAhnn2IuYWyb2BviF7xRTYLyLe1VAlGJe', '1776824189');
