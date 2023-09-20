### Set-Up
- Install docker
- In the project repository run the following command to create the docker image and start the container
- run `docker build -t maze-arch .`
- run `docker run --name=maze-game --rm --detach maze-arch`