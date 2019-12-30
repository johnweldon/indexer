.headers ON
.mode csv

SELECT
  fn.hash,
  fn.size,
  fn.path
FROM files AS fn 
ORDER BY fn.hash, fn.path
;

