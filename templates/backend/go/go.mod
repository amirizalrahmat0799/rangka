module {% go_module %}

go 1.24
{% if postgres %}

require github.com/jackc/pgx/v5 v{% v_pgx %}
{% endif %}
