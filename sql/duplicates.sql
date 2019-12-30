.headers ON
.mode csv

WITH q AS (
  SELECT 
    file_hash, 
    count(1) AS dup 
  FROM file_tags 
  WHERE tag_key = 'path'
  GROUP BY file_hash
) 
SELECT
  fn.hash,
  fn.size,
  fn.path
FROM files AS fn 
  JOIN q ON q.file_hash = fn.hash 
WHERE dup > 1
ORDER BY fn.hash, fn.path
;

