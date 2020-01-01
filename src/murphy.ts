import * as Phaser from 'phaser';

class MurphyScene extends Phaser.Scene {
  static CONFIG: Phaser.Types.Scenes.SettingsConfig = {
    active: true,
    visible: true,
    key: 'Murphy',
  };

  controls: Phaser.Cameras.Controls.FixedKeyControl | null = null;

  constructor() {
    super(MurphyScene.CONFIG);
  }

  preload() {
    this.load.image('tiles', 'levels/tiled/infotron.png');
    this.load.tilemapTiledJSON(
      'map',
      'levels/tiled/Excavation-big.tilemap.json'
    );
  }

  create() {
    var map = this.make.tilemap({key: 'map'});
    var tiles = map.addTilesetImage('infotron', 'tiles');
    var layer = map.createStaticLayer(0, tiles, 0, 0);
    this.cameras.main.setBounds(0, 0, map.widthInPixels, map.heightInPixels);
    var cursors = this.input.keyboard.createCursorKeys();
    var controlConfig = {
      camera: this.cameras.main,
      left: cursors.left,
      right: cursors.right,
      up: cursors.up,
      down: cursors.down,
      speed: 0.5,
    };
    this.controls = new Phaser.Cameras.Controls.FixedKeyControl(controlConfig);
  }

  update(time: number, delta: number) {
    this.controls!.update(delta);
  }
}

export class MurphyGame extends Phaser.Game {
  static CONFIG: Phaser.Types.Core.GameConfig = {
    title: "Murphy's Revenge",
    type: Phaser.AUTO,
    width: 640,
    height: 480,
    parent: 'game',
    backgroundColor: '#18216D',
    scene: MurphyScene,
  };

  constructor() {
    super(MurphyGame.CONFIG);
  }
}

window.onload = () => {
  var game = new MurphyGame();
};
