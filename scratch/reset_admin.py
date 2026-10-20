import sqlite3

conn = sqlite3.connect('database/brishti.db')
cur = conn.cursor()
cur.execute("DELETE FROM users WHERE role = 'admin'")
cur.execute("DELETE FROM products WHERE sku LIKE 'TEST-%'")
conn.commit()
print("Admin table and test products reset successfully")
