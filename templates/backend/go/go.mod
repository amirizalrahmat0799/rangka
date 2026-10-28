module {% go_module %}

go 1.24
{% if postgres %}

require github.com/jackc/pgx/v5 v{% v_pgx %}
{% endif %}
{% if mysql %}

require github.com/go-sql-driver/mysql v{% v_go_mysql %}
{% endif %}
