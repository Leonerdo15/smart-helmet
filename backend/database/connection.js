const pg = require('pg');


const connectionString = process.env.DATABASE_URL;
if (!connectionString) {
    throw new Error("Set DATABASE_URL before starting the backend");
}
const Pool = pg.Pool
const pool = new Pool({
    connectionString,
    max: 10
})

module.exports = pool;