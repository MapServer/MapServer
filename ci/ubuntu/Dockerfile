FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install sudo and basic dependencies required by setup.sh
RUN apt-get update --allow-insecure-repositories && \
    apt-get install -y --no-install-recommends ca-certificates ubuntu-keyring && \
    apt-get update && \
    apt-get install -y sudo curl wget gnupg lsb-release software-properties-common git && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root user 'runner' (to simulate GitHub Actions runner)
RUN useradd -m runner && echo "runner ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER runner
WORKDIR /home/runner/work/MapServer/MapServer

# Copy setup script and dependencies first to leverage Docker cache
COPY --chown=runner:runner ci/ubuntu/setup.sh ci/ubuntu/setup.sh
COPY --chown=runner:runner msautotest msautotest
RUN mkdir -p src

# Run setup script to install dependencies
RUN chmod +x ci/ubuntu/setup.sh && ci/ubuntu/setup.sh

# Copy the rest of the project
COPY --chown=runner:runner . .

# Ensure the build script is executable and run it
RUN chmod +x ci/ubuntu/build.sh
CMD ["ci/ubuntu/build.sh"]
