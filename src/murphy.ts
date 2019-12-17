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
    console.log('PRELOAD 1');
    this.load.image('tiles', 'levels/tiled/infotron.png');
    this.load.tilemapTiledJSON('map', 'levels/tiled/Excavation.tilemap.json');
    console.log('PRELOAD 2');
  }

  create() {
    console.log('CREATE 1');
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
    console.log('CREATE 2');
  }

  update(time: number, delta: number) {
    console.log('UPDATE 1');
    this.controls!.update(delta);
    console.log('UPDATE 2');
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

// export class MurphyGame {
//   game: Phaser.Game;
//   controls: Phaser.Cameras.Controls.FixedKeyControl | null = null;

//   buildConfig: () => Phaser.Types.Core.GameConfig = () => ({
//     title: "Murphy's Revenge",
//     type: Phaser.AUTO,
//     width: 640,
//     height: 480,
//     parent: 'game',
//     backgroundColor: '#18216D',
//     scene: {
//       preload: this.preload,
//       create: this.create,
//       update: this.update,
//     },
//   });

//   preload = () => {
//     this.game.load.image('tiles', 'assets/tilemaps/tiles/cybernoid.png');
//     this.game.load.tilemapTiledJSON(
//       'map',
//       'assets/tilemaps/maps/cybernoid.json'
//     );
//   };

//   create = () => {};

//   update = (time, delta) => {};

//   constructor() {
//     const config = this.buildConfig();
//     this.game = new Phaser.Game(config);

//     super(MurphyGame.config);
//   }
// }

window.onload = () => {
  var game = new MurphyGame();
};
