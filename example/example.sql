create table example(comment text);

insert into example values('hello world');
insert into example values('hasegawa! xss xss');
insert into example values('hasegawa! xss xss');
insert into example values('hasegawa! xss xss');

create trigger example_inserted
before
  insert on example
begin
  select growl(new.comment);
end;
