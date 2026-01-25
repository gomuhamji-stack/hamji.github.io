from flask import Flask, render_template, request, jsonify
import sqlite3
import os

app = Flask(__name__)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.join(BASE_DIR, 'habit_data.db')

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db_connection()
    # 기존 습관 테이블
    conn.execute('''CREATE TABLE IF NOT EXISTS habits (
        date TEXT PRIMARY KEY, habit1 INTEGER DEFAULT 0, habit2 INTEGER DEFAULT 0, 
        habit3 INTEGER DEFAULT 0, diary TEXT DEFAULT '')''')
    # 설정 테이블
    conn.execute('''CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT)''')
    # 월말 총평 테이블 추가
    conn.execute('''CREATE TABLE IF NOT EXISTS reports (year_month TEXT PRIMARY KEY, content TEXT)''')
    
    conn.execute("INSERT OR IGNORE INTO settings (key, value) VALUES ('h1_name', '습관1')")
    conn.execute("INSERT OR IGNORE INTO settings (key, value) VALUES ('h2_name', '습관2')")
    conn.execute("INSERT OR IGNORE INTO settings (key, value) VALUES ('h3_name', '습관3')")
    conn.commit()
    conn.close()

@app.route('/')
def index(): return render_template('index.html')

@app.route('/api/get_all_data/<year_month>')
def get_all_data(year_month):
    conn = get_db_connection()
    habits = conn.execute("SELECT * FROM habits WHERE date LIKE ?", (f'{year_month}%',)).fetchall()
    settings = conn.execute("SELECT * FROM settings").fetchall()
    # 총평 데이터 추가 로드
    report = conn.execute("SELECT content FROM reports WHERE year_month = ?", (year_month,)).fetchone()
    conn.close()
    return jsonify({
        "habits": [dict(h) for h in habits],
        "settings": {s['key']: s['value'] for s in settings},
        "report": report['content'] if report else ""
    })

@app.route('/api/save_report', methods=['POST'])
def save_report():
    data = request.json
    conn = get_db_connection()
    conn.execute("INSERT INTO reports (year_month, content) VALUES (?, ?) ON CONFLICT(year_month) DO UPDATE SET content=excluded.content", 
                 (data['year_month'], data['content']))
    conn.commit()
    conn.close()
    return jsonify({"status": "success"})

# ... 기존 save_habit, save_setting, get_months API는 동일하게 유지 ...
@app.route('/api/save_habit', methods=['POST'])
def save_habit():
    data = request.json
    conn = get_db_connection()
    conn.execute('''INSERT INTO habits (date, habit1, habit2, habit3, diary) VALUES (?,?,?,?,?)
        ON CONFLICT(date) DO UPDATE SET habit1=excluded.habit1, habit2=excluded.habit2, 
        habit3=excluded.habit3, diary=excluded.diary''', 
        (data['date'], data['habit1'], data['habit2'], data['habit3'], data['diary']))
    conn.commit()
    conn.close()
    return jsonify({"status": "success"})

@app.route('/api/save_setting', methods=['POST'])
def save_setting():
    data = request.json
    conn = get_db_connection()
    conn.execute("UPDATE settings SET value = ? WHERE key = ?", (data['value'], data['key']))
    conn.commit()
    conn.close()
    return jsonify({"status": "success"})

@app.route('/api/get_months')
def get_months():
    conn = get_db_connection()
    rows = conn.execute("SELECT DISTINCT strftime('%Y-%m', date) as month FROM habits ORDER BY month DESC").fetchall()
    conn.close()
    return jsonify([row['month'] for row in rows])

if __name__ == '__main__':
    init_db()
    app.run(host='127.0.0.1', port=5000, debug=True)