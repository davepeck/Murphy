import * as Phaser from 'phaser';

const config: Phaser.Types.Core.GameConfig = {
  title: "Murphy's Revenge",
  type: Phaser.AUTO,
  width: 640,
  height: 480,
  parent: 'game',
  backgroundColor: '#18216D',
};

export class MurphyGame extends Phaser.Game {
  constructor(config: Phaser.Types.Core.GameConfig) {
    super(config);
  }
}

window.onload = () => {
  var game = new MurphyGame(config);
};
