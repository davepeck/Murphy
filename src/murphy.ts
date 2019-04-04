import "phaser";

const config: GameConfig = {
  title: "Murphy's Revenge",
  width: 640,
  height: 480,
  parent: "game",
  backgroundColor: "#18216D"
};

export class MurphyGame extends Phaser.Game {
  constructor(config: GameConfig) {
    super(config);
  }
}

window.onload = () => {
  var game = new MurphyGame(config);
};
