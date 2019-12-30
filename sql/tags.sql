.headers ON
.mode csv

SELECT
  tag_key,
  tag_val,
  count(1) num
FROM file_tags
GROUP BY tag_key, tag_val
ORDER BY num,tag_key
;

