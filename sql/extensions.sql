SELECT
  tag_val,
  count(1) num
FROM file_tags
WHERE tag_key = 'ext'
GROUP BY tag_val
ORDER BY num,tag_key
;

