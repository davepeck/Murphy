#!npx ts-node

/**
 * @file converters/convert.ts
 *
 * The primary conversion executable script. Run as:
 *
 * ./convert.ts --input-dir ./foo --output-dir ./bar --infotron
 * or
 * ./convert.ts --input-dir ./foo --output-dir ./bar --mr
 */

import {promisify} from 'util';
import fs from 'fs';
import path from 'path';

import {ArgumentParser} from 'argparse';
import {SupaplexLevel} from '../schema';
import {LevelConverter} from './shared';
import {InfotronLevel, convert as convertInfotronLevel} from './infotron';
import {
  MurphysRevengeLevel,
  convert as convertMurphysRevengeLevel,
} from './murphy';

const directoryExists = async (path: string): Promise<boolean> => {
  try {
    await promisify(fs.stat)(path);
  } catch (err) {
    return false;
  }
  return true;
};

enum InputLevelType {
  Infotron,
  MurphysRevenge,
}

const processArguments = () => {
  const parser = new ArgumentParser({
    version: '0.0.1',
    addHelp: true,
    description: 'Private to Supaplex JSON format converter',
  });
  parser.addArgument(['-I', '--input-dir'], {
    help: 'Directory containing Private-format JSON files',
    required: true,
  });
  parser.addArgument(['-O', '--output-dir'], {
    help: 'Directory to store converted Supaplex-format JSON files',
    required: true,
  });
  parser.addArgument(['-i', '--infotron'], {
    help: 'If provided, input files are assumed to be infotron levels',
    action: 'storeTrue',
    defaultValue: false,
  });
  parser.addArgument(['-m', '--murphy'], {
    help: 'If provided, input files are assumed to be Murphys Revenge levels',
    action: 'storeTrue',
    defaultValue: false,
  });

  const args = parser.parseArgs();

  const inputDir = path.resolve(args.input_dir);
  const outputDir = path.resolve(args.output_dir);
  const inputLevelType = args.infotron
    ? InputLevelType.Infotron
    : InputLevelType.MurphysRevenge;

  return {inputDir, outputDir, inputLevelType};
};

const readInputFile = async <T>(
  fileName: string,
  converter: LevelConverter<T>
): Promise<SupaplexLevel> => {
  const jsonString = await promisify(fs.readFile)(fileName, 'utf8');
  const level: T = JSON.parse(jsonString) as T;
  return converter(level);
};

const writeOutputFile = async (
  outputFileName: string,
  level: SupaplexLevel
) => {
  const jsonString = JSON.stringify(level, null, 2);
  await promisify(fs.writeFile)(outputFileName, jsonString, 'utf8');
};

const main = async () => {
  const {inputDir, outputDir, inputLevelType} = processArguments();
  try {
    const fileNames = await promisify(fs.readdir)(inputDir);
    for (const fileName of fileNames) {
      const inputFileName = path.join(inputDir, fileName);
      const baseFileName = path.basename(fileName, '.json');
      const outputFileName = path.join(
        outputDir,
        `./${baseFileName}.supaplex.json`
      );
      console.log(`Converting ${inputFileName} to ${outputFileName}`);
      try {
        const supaplexLevel = await (inputLevelType === InputLevelType.Infotron
          ? readInputFile(inputFileName, convertInfotronLevel)
          : readInputFile(inputFileName, convertMurphysRevengeLevel));
        await writeOutputFile(outputFileName, supaplexLevel);
      } catch (err) {
        console.error(`FAILURE processing ${inputFileName}: ${err}`);
      }
    }
  } catch (err) {
    console.error(err);
    return;
  }
};

main();
