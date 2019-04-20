# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

# == Distribution preparation ==
ARG DIST
FROM $DIST

WORKDIR /usr/src/beast

# make sure getpwuid() works, and ~/ is writable, otherwise many tools break
ARG USERGROUP
RUN : && \
  mkdir -m 0755 -p /home/cibuilder/.electron/ && \
  groupadd --gid ${USERGROUP#*:} cibuilder && \
  useradd --uid ${USERGROUP%:*} --gid ${USERGROUP#*:} --home-dir /home/cibuilder cibuilder && \
  { ! compgen -G "/root/.electron/*.zip" >/dev/null || \
      cp --reflink=auto --preserve=timestamps /root/.electron/*.zip /home/cibuilder/.electron/ ; } && \
  chown -R ${USERGROUP} /home/cibuilder
# Also copies pre-fetched electron download

COPY .cicache/electron/ /home/cibuilder/.electron/
