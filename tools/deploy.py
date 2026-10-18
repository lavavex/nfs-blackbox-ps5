#!/usr/bin/env python3
"""Deploy built app folders to the PS5 over FTP (ftpsrv, port 2121, no login).

  PS5_HOST=<console address> tools/deploy.py [--host ADDRESS] [--port 2121] [--all] [--dry-run] APP_DIR...

Each APP_DIR (a folder holding eboot.bin and sce_sys/param.json, named after nothing in
particular: its title comes from param.json) goes to /data/homebrew/<TITLEID>:
  - only files that differ are sent (size on the console, or a changed SHA-256 since the
    last deploy from this machine: logs/deploy/<host>-<TITLEID>.json); --all sends all;
  - each file is sent under a temporary name and renamed into place; eboot.bin and
    sce_sys/param.json go last, so a half-deployed app never looks complete;
  - then every folder and file of the app is made 777 (ShadowMount+ needs it);
  - then the console's listing is checked: every file there with the local size and 777.
Files on the console that the build no longer has are listed, never removed.
Close the app on the console first: a running app's files can't be replaced.

Works the same on macOS and Linux (python3 only), e.g.
  tools/deploy.py ../prospero-blackbox/dist/PPSA99400
  tools/deploy.py release/apps/PPSA99403
"""
import argparse, ftplib, hashlib, json, os, posixpath, re, sys, time

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
ap.add_argument('apps', nargs='+')
ap.add_argument('--host', default=os.environ.get('PS5_HOST'), required=not os.environ.get('PS5_HOST'),
                help='the console (or PS5_HOST)')
ap.add_argument('--port', type=int, default=int(os.environ.get('PS5_PORT', '2121')))
ap.add_argument('--all', action='store_true', help='send every file')
ap.add_argument('--dry-run', action='store_true', help='compare only; change nothing on the console')
args = ap.parse_args()
CRITICAL = ('sce_sys/param.json', 'eboot.bin')  # sent last, in this order


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def connect():
    ftp = ftplib.FTP()
    ftp.connect(args.host, args.port, timeout=60)
    ftp.login()
    ftp.voidcmd('TYPE I')
    return ftp


FACTS = [['type', 'size', 'unix.mode']]  # [] once a server refuses to be asked


def remote_tree(ftp, root):
    """{relative path: (size, mode)} for files, and the set of folders, below root; None
    when root isn't there."""
    files, dirs, stack = {}, set(), ['']
    while stack:
        rel = stack.pop()
        path = posixpath.join(root, rel) if rel else root
        try:
            try:
                entries = list(ftp.mlsd(path, FACTS[0]))
            except ftplib.error_perm as e:
                # ftpsrv refuses OPTS MLST (504) but lists the mode anyway; others need asking.
                if not (FACTS[0] and str(e).startswith('504')):
                    raise
                FACTS[0] = []
                entries = list(ftp.mlsd(path))
        except ftplib.error_perm:
            if not rel:
                return None, None
            raise
        for name, facts in entries:
            if name in ('.', '..') or facts.get('type') in ('cdir', 'pdir'):
                continue
            path = posixpath.join(rel, name) if rel else name
            if facts.get('type') == 'dir':
                dirs.add(path)
                stack.append(path)
            else:
                files[path] = (int(facts.get('size', -1)), open_to_all(facts.get('unix.mode', '')))
    return files, dirs


def open_to_all(mode):
    """Whether an MLSD unix.mode ("0777", or "0o777" elsewhere) is 777."""
    try:
        return int(mode.replace('o', ''), 8) & 0o777 == 0o777
    except ValueError:
        return False


def ignore_missing(call, *a):
    try:
        call(*a)
    except ftplib.error_perm:
        pass


def deploy(app):
    param = os.path.join(app, 'sce_sys', 'param.json')
    if not (os.path.isfile(os.path.join(app, 'eboot.bin')) and os.path.isfile(param)):
        sys.exit(f'not an app folder (eboot.bin, sce_sys/param.json): {app}')
    title = json.load(open(param))['titleId']
    if not re.fullmatch(r'PPSA\d{5}', title):
        sys.exit(f'{param}: bad titleId {title!r}')
    root = '/data/homebrew/' + title
    local = {}
    for folder, dirs, names in os.walk(app):
        dirs.sort()
        for name in names:
            path = os.path.join(folder, name)
            if name == '.DS_Store' or os.path.islink(path):
                continue
            local[os.path.relpath(path, app).replace(os.sep, '/')] = path
    record_path = os.path.join(R, 'logs', 'deploy', f'{args.host}-{title}.json')
    try:
        record = json.load(open(record_path))
    except (OSError, ValueError):
        record = {}
    digests = {rel: sha256(path) for rel, path in local.items()}

    ftp = connect()
    there, _ = remote_tree(ftp, root)
    there = there or {}
    todo = [rel for rel in sorted(local, key=lambda r: (r in CRITICAL, CRITICAL.index(r) if r in CRITICAL else 0, r))
            if args.all or there.get(rel, (None,))[0] != os.path.getsize(local[rel]) or record.get(rel) != digests[rel]]
    extra = sorted(set(there) - set(local))
    size = sum(os.path.getsize(local[rel]) for rel in todo)
    print(f'{title}: {len(local)} files in {os.path.relpath(app)}; {len(todo)} to send '
          f'({size / 2**20:.1f} MiB) -> {args.host}:{root}')
    for rel in extra:
        print(f'  only on the console (left as it is): {rel}')
    if args.dry_run:
        ftp.quit()
        return True

    made = set()
    def makedirs(path):
        parts = path.strip('/').split('/')
        for i in range(1, len(parts) + 1):
            d = '/' + '/'.join(parts[:i])
            if d not in made:
                ignore_missing(ftp.mkd, d)
                made.add(d)

    sent, start = 0, time.time()
    for rel in todo:
        remote = posixpath.join(root, rel)
        temporary = posixpath.join(posixpath.dirname(remote), '.' + posixpath.basename(remote) + '.upload')
        makedirs(posixpath.dirname(remote))
        def progress(chunk):
            nonlocal sent
            sent += len(chunk)
            done = sent / (size or 1)
            sys.stdout.write(f"\r  [{'#' * int(30 * done):<30}] {100 * done:5.1f}%  {rel[-48:]:<48}")
            sys.stdout.flush()
        try:
            with open(local[rel], 'rb') as f:
                ftp.storbinary(f'STOR {temporary}', f, blocksize=1 << 20, callback=progress)
            ignore_missing(ftp.voidcmd, f'SITE CHMOD 777 {remote}')  # a file the app wrote
            ignore_missing(ftp.sendcmd, f'DELE {remote}')  # ftpsrv answers 226
            ftp.rename(temporary, remote)
        except ftplib.all_errors as e:
            print(f'\n{rel}: {e}\n(is the app running on the console? close it and deploy again)')
            return False
        record[rel] = digests[rel]
        os.makedirs(os.path.dirname(record_path), exist_ok=True)
        with open(record_path, 'w') as f:  # kept as it goes: an interrupted deploy resumes
            json.dump(record, f, indent=0, sort_keys=True)
    if todo:
        print(f'\n  sent in {time.time() - start:.0f} s')

    # Everything 777, folders too, then the console's own listing checked.
    files, dirs = remote_tree(ftp, root)
    for path in [root] + [posixpath.join(root, d) for d in sorted(dirs)] + \
                [posixpath.join(root, f) for f in sorted(files) if not files[f][1]]:
        ftp.voidcmd(f'SITE CHMOD 777 {path}')
    files, dirs = remote_tree(ftp, root)
    ftp.quit()
    wrong = [rel for rel in local if files.get(rel, (None,))[0] != os.path.getsize(local[rel])]
    modes = [rel for rel in local if rel in files and not files[rel][1]]
    for rel in wrong:
        print(f'  WRONG on the console: {rel} ({files.get(rel, ("missing",))[0]} vs {os.path.getsize(local[rel])})')
    for rel in modes:
        print(f'  not 777 on the console: {rel}')
    if wrong or modes:
        return False
    print(f'  verified: all {len(local)} files on the console at their sizes, 777')
    return True


ok = all([deploy(app) for app in args.apps])
sys.exit(0 if ok else 1)
